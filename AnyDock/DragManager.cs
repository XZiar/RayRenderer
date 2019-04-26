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
        internal readonly AnyDockPanel Panel;
        internal DragData(UIElement source)
        {
            Element = source;
            Panel = AnyDockManager.GetParentDock(Element);
        }
    }
    internal interface IDragRecievePoint
    {
        AnyDockPanel ParentDockPoint { get; }
        void OnDragIn(DragData data, Point pos);
        void OnDragOut(DragData data, Point pos);
        void OnDragDrop(DragData data, Point pos);
    }
    static class DragManager
    {
        private static readonly ConditionalWeakTable<AnyDockPanel, Window> WindowTable = new ConditionalWeakTable<AnyDockPanel, Window>();
        private static readonly Dictionary<Window, uint> ReferenceTable = new Dictionary<Window, uint>();
        internal static void RegistDragHost(AnyDockPanel panel)
        {
            var window = Window.GetWindow(panel);
            WindowTable.Add(panel, window);
            ReferenceTable[window] = ReferenceTable.TryGetValue(window, out uint count) ? count + 1 : 1;
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
            var winMap = ReferenceTable.Keys.ToDictionary(win => new WindowInteropHelper(win).Handle);
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
            data.Panel.Children.Remove(data.Element);
            ZOrderWindows = GetZOrderWindows().ToArray();
            var dragWindow = new DragHostWindow(windowPos, deltaPoint, data.Element, data);
            dragWindow.Draging += OnDraging;
            dragWindow.Draged += OnDraged;
            dragWindow.Show();
        }

        private static Window[] ZOrderWindows;
        private static IDragRecievePoint FindDropPoint(DependencyObject element)
        {
            while (element != null)
            {
                if (element is IDragRecievePoint target && (target.ParentDockPoint?.AllowDropTab ?? false))
                    return target;
                element = VisualTreeHelper.GetParent(element);
            }
            return null;
        }
        private static IDragRecievePoint LastDragPoint = null;

        private static void ProbeDrag(Point screenPos, DragData data)
        {
            IDragRecievePoint target = null;
            //Console.WriteLine($"Drag Over [{screenPos}] [{Mouse.DirectlyOver}]");
            //if (Mouse.DirectlyOver is DependencyObject ele)
            //{
            //    var targetWindow = Window.GetWindow(ele);
            //    if (ReferenceTable.ContainsKey(targetWindow))
            //    {
            //        var relPos = targetWindow.PointFromScreen(screenPos);
            //        var hitPart = targetWindow.InputHitTest(relPos) as UIElement;
            //        target = FindDropPoint(hitPart);
            //    }
            //}
            foreach (var window in ZOrderWindows)
            {
                var relPos = window.PointFromScreen(screenPos);
                var hitPart = window.InputHitTest(relPos) as UIElement;
                target = FindDropPoint(hitPart);
                if (target != null) break;
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
        private static void OnDraging(DragHostWindow window, Point screenPos, DragData data)
        {
            ProbeDrag(screenPos, data);
        }
        private static void OnDraged(DragHostWindow window, Point screenPos, DragData data)
        {
            window.Content = null;
            ZOrderWindows = null;
            //ProbeDrag(screenPos, data);
            //LoacationChanged(Draging) must happen before DragMove finished(Drop)
            if (LastDragPoint != null)
            {
                LastDragPoint.OnDragDrop(data, ((FrameworkElement)LastDragPoint).PointFromScreen(screenPos));
                LastDragPoint = null;
            }
            else
            {
                data.Panel.Children.Add(data.Element);
            }
        }
    }
}
