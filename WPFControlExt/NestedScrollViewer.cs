using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using XZiar.Util;

namespace XZiar.WPFControl
{
    public class NestedScrollViewer : ScrollViewer
    {
        private const double Epsilon = 0.00001;
        public static bool AlmostZero(double val)
        {
            if (val == 0)
                return true;
            var delta = val - 0;
            return (delta < Epsilon) && (delta > -Epsilon);
        }

        private class PanningInfoWrap
        {
            private static readonly FieldInfo ThePanningInfo;
            private static readonly PropertyInfo UnusedTranslationProp;
            private static readonly PropertyInfo IsPanningProp;
            static PanningInfoWrap()
            {
                ThePanningInfo = typeof(ScrollViewer).GetField("_panningInfo", BindingFlags.NonPublic | BindingFlags.Instance);
                var PanningInfoType = typeof(ScrollViewer).GetNestedType("PanningInfo", BindingFlags.NonPublic);
                UnusedTranslationProp = PanningInfoType.GetProperty("UnusedTranslation");
                IsPanningProp = PanningInfoType.GetProperty("IsPanning");
            }

            private readonly object Info;
            public Vector UnusedTranslation
            {
                get => (Vector)UnusedTranslationProp.GetValue(Info);
                set => UnusedTranslationProp.SetValue(Info, value);
            }
            public bool IsPanning
            {
                get => (bool)IsPanningProp.GetValue(Info);
                set => IsPanningProp.SetValue(Info, value);
            }
            private PanningInfoWrap(object info)
            {
                Info = info;
            }

            public static PanningInfoWrap From(ScrollViewer view)
            {
                var info = ThePanningInfo.GetValue(view);
                return info == null ? null : new PanningInfoWrap(info);
            }
        }


        //private delegate Vector GetUnusedTranslationDelegate(object panningInfo);
        //private delegate void SetUnusedTranslationDelegate(object panningInfo, Vector value);
        private delegate void ManipulateScrollDelegate(ScrollViewer view, double delta, double cumulativeTranslation, bool isHorizontal);
        private delegate void CanStartScrollManipulationDelegate(ScrollViewer view, Vector translation, out bool cancelManipulation);

        //private static readonly GetUnusedTranslationDelegate GetUnusedTranslation;
        //private static readonly SetUnusedTranslationDelegate SetUnusedTranslation;
        private static readonly ManipulateScrollDelegate ManipulateScroll;
        private static readonly CanStartScrollManipulationDelegate CanStartScrollManipulation;

        private delegate bool GetManipulationRequested(ManipulationDeltaEventArgs arg);
        private delegate void SetManipulationRequested(ManipulationDeltaEventArgs arg, bool isRequested);
        private static readonly GetManipulationRequested ReqCancelGetter;
        private static readonly SetManipulationRequested ReqCancelSetter;
        private static readonly GetManipulationRequested ReqCompleteGetter;
        private static readonly SetManipulationRequested ReqCompleteSetter;
        private static readonly ConstructorInfo ManipulationStartConstructor;
        private static readonly ConstructorInfo ManipulationDeltaConstructor;
        private static readonly ConstructorInfo ManipulationCompleteConstructor;

        private class BypassDeltaEventArgs : RoutedEventArgs
        {
            internal Vector UnusedTranslation;
            public BypassDeltaEventArgs(Vector unused) : base(BypassDeltaEvent) { UnusedTranslation = unused; }
        }
        private static readonly RoutedEvent BypassDeltaEvent = EventManager.RegisterRoutedEvent(
            "BypassDelta",
            RoutingStrategy.Bubble,
            typeof(EventHandler<BypassDeltaEventArgs>),
            typeof(NestedScrollViewer));

        private static readonly RoutedEvent BypassManipulationStartEvent = EventManager.RegisterRoutedEvent(
            "BypassManipulationStart",
            RoutingStrategy.Bubble,
            typeof(InputEventHandler),
            typeof(NestedScrollViewer));
        //private static readonly RoutedEvent BypassManipulationDeltaEvent = EventManager.RegisterRoutedEvent(
        //    "BypassManipulationDelta",
        //    RoutingStrategy.Bubble,
        //    typeof(InputEventHandler),
        //    typeof(NestedScrollViewer));
        private static readonly RoutedEvent BypassManipulationCompleteEvent = EventManager.RegisterRoutedEvent(
            "BypassManipulationComplete",
            RoutingStrategy.Bubble,
            typeof(InputEventHandler),
            typeof(NestedScrollViewer));

        static NestedScrollViewer()
        {
            var mths = typeof(ScrollViewer).GetMethods(BindingFlags.NonPublic | BindingFlags.Instance);
            ManipulateScroll = mths
                .Where(x => x.Name == "ManipulateScroll" && x.GetParameters().Length == 3).First()
                .ToDelegate<ManipulateScrollDelegate>();
            CanStartScrollManipulation = mths
                .Where(x => x.Name == "CanStartScrollManipulation").First()
                .ToDelegate<CanStartScrollManipulationDelegate>();


            var propCancel = typeof(ManipulationDeltaEventArgs)
                .GetProperty("RequestedCancel", BindingFlags.Instance | BindingFlags.NonPublic);
            var propComplete = typeof(ManipulationDeltaEventArgs)
                .GetProperty("RequestedComplete", BindingFlags.Instance | BindingFlags.NonPublic);
            ReqCancelGetter = (GetManipulationRequested)Delegate.CreateDelegate(typeof(GetManipulationRequested),
                propCancel.GetGetMethod(true));
            ReqCancelSetter = (SetManipulationRequested)Delegate.CreateDelegate(typeof(SetManipulationRequested),
                propCancel.GetSetMethod(true));
            ReqCompleteGetter = (GetManipulationRequested)Delegate.CreateDelegate(typeof(GetManipulationRequested),
                propComplete.GetGetMethod(true));
            ReqCompleteSetter = (SetManipulationRequested)Delegate.CreateDelegate(typeof(SetManipulationRequested),
                propComplete.GetSetMethod(true));


            ManipulationStartConstructor = typeof(ManipulationStartingEventArgs).GetConstructors(BindingFlags.Instance | BindingFlags.NonPublic)[0];
            ManipulationDeltaConstructor = typeof(ManipulationDeltaEventArgs).GetConstructors(BindingFlags.Instance | BindingFlags.NonPublic)[0];
            ManipulationCompleteConstructor = typeof(ManipulationCompletedEventArgs).GetConstructors(BindingFlags.Instance | BindingFlags.NonPublic)[0];

            EventManager.RegisterClassHandler(typeof(NestedScrollViewer), BypassManipulationStartEvent, 
                new InputEventHandler(OnBypassManipulationStart));
            EventManager.RegisterClassHandler(typeof(NestedScrollViewer), BypassManipulationCompleteEvent,
                new InputEventHandler(OnBypassManipulationComplete));


            EventManager.RegisterClassHandler(typeof(NestedScrollViewer), BypassDeltaEvent, new EventHandler<BypassDeltaEventArgs>(OnBypassDelta));
        }

        private static void OnBypassDelta(object sender, BypassDeltaEventArgs e)
        {
            var self = (NestedScrollViewer)sender;
            if (self.CurPanningInfo != null) // can accept
            {
                var prevUnused = self.CurPanningInfo.UnusedTranslation;
                if (self.PanningMode != PanningMode.VerticalOnly)
                {
                    // Scroll horizontally unless the mode is VerticalOnly
                    ManipulateScroll(self, e.UnusedTranslation.X, e.UnusedTranslation.X, true);
                }
                if (self.PanningMode != PanningMode.HorizontalOnly)
                {
                    // Scroll vertically unless the mode is HorizontalOnly
                    ManipulateScroll(self, e.UnusedTranslation.Y, e.UnusedTranslation.Y, false);
                }
                var curUnused = self.CurPanningInfo.UnusedTranslation;
                var used = curUnused - prevUnused;
                if (!AlmostZero(used.X) || !AlmostZero(used.Y))
                {
                    e.Handled = true;
                    e.UnusedTranslation -= used;
                }
            }
        }

        private static void OnBypassManipulationStart(object sender, InputEventArgs e)
        {
            var self = (NestedScrollViewer)sender;
            self.OnManipulationStarting((ManipulationStartingEventArgs)e);
            self.CurPanningInfo = PanningInfoWrap.From(self);
        }

        private static void OnBypassManipulationComplete(object sender, InputEventArgs e)
        {
            var self = (NestedScrollViewer)sender;
            if (self.CurPanningInfo != null)
            {
                self.OnManipulationCompleted((ManipulationCompletedEventArgs)e);
                self.CurPanningInfo = null;
            }
        }

        protected override void OnMouseWheel(MouseWheelEventArgs e)
        {
            var oldVOffset = VerticalOffset;
            var oldHOffset = HorizontalOffset;
            base.OnMouseWheel(e);
            if (oldVOffset == VerticalOffset && oldHOffset == HorizontalOffset)
                e.Handled = false; // continue bubble
        }

        private IInputElement CurReciever;
        private bool HandleToParent = false;
        private bool SeenTapGesture = false;
        private PanningInfoWrap CurPanningInfo;

        protected override void OnStylusSystemGesture(StylusSystemGestureEventArgs e)
        {
            SeenTapGesture = e.SystemGesture == SystemGesture.Tap;
            base.OnStylusSystemGesture(e);
        }
        protected override void OnManipulationStarting(ManipulationStartingEventArgs e)
        {
            base.OnManipulationStarting(e);
            SeenTapGesture = false;
            CurPanningInfo = PanningInfoWrap.From(this);
            if (e.Handled && CurPanningInfo != null)
            {
                var e2 = (ManipulationStartingEventArgs)ManipulationStartConstructor.Invoke(new object[] { e.Device, e.Timestamp });
                e2.RoutedEvent = BypassManipulationStartEvent;
                ((UIElement)Parent).RaiseEvent(e2);
                CurReciever = e2.ManipulationContainer;
            }
        }
        protected override void OnManipulationDelta(ManipulationDeltaEventArgs e)
        {
            if (!HandleToParent)
            {
                var scrollable = new Vector(ScrollableWidth, ScrollableHeight);
                base.OnManipulationDelta(e);
                if (!e.Handled || SeenTapGesture || CurPanningInfo == null) // not handled or tap
                    return;
                if (ReqCompleteGetter(e))
                {
                    Console.WriteLine($"Reported Complete");
                    HandleToParent = true;
                }
                else if (ReqCancelGetter(e))
                {
                    Console.WriteLine($"Reported Cancel");
                    HandleToParent = true;
                }
                else if (!CurPanningInfo.IsPanning) // has not trigger panning
                    return;

                Vector unused = CurPanningInfo.UnusedTranslation;
                if (AlmostZero(scrollable.X))
                    unused.X = e.DeltaManipulation.Translation.X;
                if (AlmostZero(scrollable.Y))
                    unused.Y = e.DeltaManipulation.Translation.Y;
                if (AlmostZero(unused.X) && AlmostZero(unused.Y))
                    return;

                Console.WriteLine($"Has unused [{unused}]");
                var deltaArg = new BypassDeltaEventArgs(unused);
                ((UIElement)Parent).RaiseEvent(deltaArg);
                if (!deltaArg.Handled) // noone handled, let it pass
                    HandleToParent = false;
                else
                {
                    CurPanningInfo.UnusedTranslation = deltaArg.UnusedTranslation;
                    if (HandleToParent) // fix event state to keep recieve delta
                    {
                        ReqCompleteSetter(e, false);
                        ReqCancelSetter(e, false);
                    }
                }
            }
            else
            {
                e.Handled = true; // force keep recieving event since they need to be handled to parent
                Vector unused = CurPanningInfo.UnusedTranslation + e.DeltaManipulation.Translation;
                var deltaArg = new BypassDeltaEventArgs(unused);
                ((UIElement)Parent).RaiseEvent(deltaArg);
                if (!deltaArg.Handled) // noone handled, finish
                {
                    HandleToParent = false;
                    ReqCompleteSetter(e, true);
                }
                else
                    CurPanningInfo.UnusedTranslation = deltaArg.UnusedTranslation;
            }
        }
        protected override void OnManipulationCompleted(ManipulationCompletedEventArgs e)
        {
            base.OnManipulationCompleted(e);

            var e2 = (ManipulationCompletedEventArgs)ManipulationCompleteConstructor.Invoke(
                new object[] { e.Device, e.Timestamp, CurReciever, e.ManipulationOrigin, e.TotalManipulation, e.FinalVelocities, e.IsInertial });
            e2.RoutedEvent = BypassManipulationCompleteEvent;
            ((UIElement)Parent).RaiseEvent(e2);
            CurReciever = null;
            HandleToParent = false;
        }

    }
}
