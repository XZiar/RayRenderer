using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace AnyDock
{
    internal interface IDragRecievePoint
    {
        AnyDockPanel ParentDockPoint { get; }
        void OnDragIn(DragData data, Point pos);
        void OnDragOut(DragData data, Point pos);
        void OnDragDrop(DragData data, Point pos);
    }
    class DragManager
    {
        private static Window SourceWindow = null;
        internal static void PerformDrag(Point windowPos, Point deltaPoint, DragData data)
        {
            if (!data.AllowDrag)
                return;
            SourceWindow = Window.GetWindow(data.Element);
            data.Panel.Children.Remove(data.Element);
            var dragWindow = new DragHostWindow(windowPos, deltaPoint, data.Element, data);
            dragWindow.Draging += OnDraging;
            dragWindow.Draged += OnDraged;
            dragWindow.Show();
        }

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
            var relPos = SourceWindow.PointFromScreen(screenPos);
            //Console.WriteLine($"Diff Pos [{screenPos}]:[{relPos}]");
            var hitPart = SourceWindow.InputHitTest(relPos) as UIElement;
            var target = FindDropPoint(hitPart);
            if (LastDragPoint != target)
            {
                LastDragPoint?.OnDragOut(data, ((FrameworkElement)LastDragPoint).PointFromScreen(screenPos));
                target?.OnDragIn(data, ((FrameworkElement)target).PointFromScreen(screenPos));
                Console.WriteLine($"Prob [{hitPart}] [{LastDragPoint}] [{target}]");
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
            ProbeDrag(screenPos, data);
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
