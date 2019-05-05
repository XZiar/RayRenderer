using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;

namespace AnyDock
{
    internal class DragData
    {
        internal readonly UIElement Element;
        internal readonly DraggableTabControl TabRoot;
        internal DragData(UIElement source, DraggableTabControl root)
        {
            Element = source;
            TabRoot = root;
        }
    }
    internal interface IDragRecievePoint
    {
        //bool RecieveDrag();
        void OnDragIn(DragData data, Point pos);
        void OnDragOut(DragData data, Point pos);
        void OnDragDrop(DragData data, Point pos);
    }
    static class DragManager
    {
        internal class RecieveDragEventArgs : RoutedEventArgs
        {
            public readonly Point ScreenPos;
            public IDragRecievePoint RecievePoint;
            internal RecieveDragEventArgs(Point screenPos) : base(RecieveDragEvent) { ScreenPos = screenPos; }
        }
        internal delegate void RecieveDragEventHandler(UIElement sender, RecieveDragEventArgs args);
        internal static readonly RoutedEvent RecieveDragEvent = EventManager.RegisterRoutedEvent(
            "RecieveDrag",
            RoutingStrategy.Bubble,
            typeof(RecieveDragEventHandler),
            typeof(DragManager));
        internal static void AddRecieveDragHandler(UIElement element, RecieveDragEventHandler handler) =>
            element.AddHandler(RecieveDragEvent, handler);
        internal static void RemoveRecieveDragHandler(UIElement element, RecieveDragEventHandler handler) =>
            element.RemoveHandler(RecieveDragEvent, handler);

        private static readonly ConditionalWeakTable<AnyDockPanel, Window> WindowTable = new ConditionalWeakTable<AnyDockPanel, Window>();
        private static readonly Dictionary<Window, uint> ReferenceTable = new Dictionary<Window, uint>();
        internal static void RegistDragHost(AnyDockPanel panel)
        {
            var window = Window.GetWindow(panel);
            if (window != null)
            {
                WindowTable.Add(panel, window);
                ReferenceTable[window] = ReferenceTable.TryGetValue(window, out uint count) ? count + 1 : 1;
            }
        }
        internal static void UnregistDragHost(AnyDockPanel panel)
        {
            if (WindowTable.TryGetValue(panel, out Window window))
            {
                WindowTable.Remove(panel);
                var count = ReferenceTable[window] - 1;
                if (count == 0)
                    ReferenceTable.Remove(window);
                else
                    ReferenceTable[window] = count;
            }
        }

        private const uint GW_HWNDFIRST = 0;
        private const uint GW_HWNDNEXT = 2;
        //[DllImport("User32")] static extern IntPtr GetTopWindow(IntPtr hWnd);
        [DllImport("User32")] static extern IntPtr GetWindow(IntPtr hWnd, uint wCmd);
        private static IEnumerable<Window> GetZOrderWindows()
        {
            //var winMap = ReferenceTable.Keys.ToDictionary(win => new WindowInteropHelper(win).Handle);
            var winMap = Application.Current.Windows.Cast<Window>()
                .ToDictionary(win => new WindowInteropHelper(win).Handle);
            for (var hWnd = GetWindow(winMap.Keys.First(), GW_HWNDFIRST);
                hWnd != IntPtr.Zero && winMap.Count > 0;
                hWnd = GetWindow(hWnd, GW_HWNDNEXT))
            {
                if (winMap.TryGetValue(hWnd, out Window window))
                {
                    winMap.Remove(hWnd);
                    yield return window;
                }
            }
        }

        internal static void PerformDrag(Point windowPos, Point deltaPoint, DragData data)
        {
            AnyDockManager.RaiseRemovedEvent(data.Element);
            ZOrderWindows = GetZOrderWindows().ToArray();
            var dragWindow = new DraggingWindow(windowPos, deltaPoint, data);
            dragWindow.Draging += OnDraging;
            dragWindow.Draged += OnDraged;
            dragWindow.Show();
        }

        private static Window[] ZOrderWindows;
        private static IDragRecievePoint LastDragPoint = null;

        private static void ProbeDrag(Point screenPos, DragData data)
        {
            IDragRecievePoint target = null;
            var earg = new RecieveDragEventArgs(screenPos);
            foreach (var window in ZOrderWindows)
            {
                var relPos = window.PointFromScreen(screenPos);
                if (window.InputHitTest(relPos) is UIElement hitPart)
                    hitPart.RaiseEvent(earg);
                if (earg.Handled)
                {
                    target = earg.RecievePoint;
                    break;
                }
            }
            //Console.WriteLine($"Diff Pos [{screenPos}]:[{relPos}]");
            if (LastDragPoint != target)
            {
                //Console.WriteLine($"DragTarget [{LastDragPoint}]->[{target}]");
                LastDragPoint?.OnDragOut(data, ((FrameworkElement)LastDragPoint).PointFromScreen(screenPos));
                target?.OnDragIn(data, ((FrameworkElement)target).PointFromScreen(screenPos));
            }
            LastDragPoint = target;
        }
        private static void OnDraging(DraggingWindow window, Point screenPos, DragData data)
        {
            ProbeDrag(screenPos, data);
        }
        private static void OnDraged(DraggingWindow window, Point screenPos, DragData data)
        {
            ZOrderWindows = null;
            //LoacationChanged(Draging) must happen before DragMove finished(Drop)
            if (LastDragPoint != null)
            {
                LastDragPoint.OnDragDrop(data, ((FrameworkElement)LastDragPoint).PointFromScreen(screenPos));
                LastDragPoint = null;
            }
            else
            {
                var panel = new DraggableTabControl()
                {
                    TabStripPlacement = data.TabRoot.TabStripPlacement,
                    AllowDropTab = data.TabRoot.AllowDropTab
                };
                panel.RealChildren.Add(data.Element);

                var extraWidth = SystemParameters.ResizeFrameVerticalBorderWidth;
                var extraHeight = SystemParameters.WindowCaptionHeight + SystemParameters.ResizeFrameHorizontalBorderHeight;
                var newWindow = new Window
                {
                    SizeToContent = SizeToContent.Manual,
                    Width = window.Width + extraWidth, 
                    Height = window.Height + extraHeight,
                    WindowStartupLocation = WindowStartupLocation.Manual,
                    Left = window.Left - extraWidth, Top = window.Top - extraHeight,
                    Content = panel
                };
                newWindow.Show();
                //data.Panel.Children.Add(data.Element);
            }
        }

    }
}
