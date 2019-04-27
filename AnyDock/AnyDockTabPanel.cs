using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;

namespace AnyDock
{
    class AnyDockTabPanel : TabPanel, IDragRecievePoint
    {
        public static readonly DependencyProperty ParentDockProperty = DependencyProperty.Register(
            "ParentDock",
            typeof(DependencyObject),
            typeof(AnyDockTabPanel),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.None));
        internal DependencyObject ParentDock { set => SetValue(ParentDockProperty, value); }
        private Size GetDesiredSizeWithoutMargin(UIElement element)
        {
            var margin = (Thickness)element.GetValue(MarginProperty);
            var height = Math.Max(0d, element.DesiredSize.Height - margin.Top - margin.Bottom);
            var width = Math.Max(0d, element.DesiredSize.Width - margin.Left - margin.Right);
            return new Size(width, height);
        }
        private Dock TabStripPlacement
        {
            get => (GetValue(DockPanel.DockProperty) as Dock?) ?? Dock.Top;
        }
        private int SelectedIndex
        {
            get => (TemplatedParent as TabControl)?.SelectedIndex ?? -1;
        }

        public AnyDockPanel ParentDockPoint { get => (AnyDockPanel)GetValue(ParentDockProperty); }

        static AnyDockTabPanel()
        {
        }

        protected override Size MeasureOverride(Size constraint)
        {
            var old = base.MeasureOverride(constraint);
            var totalSize = new Vector(0, 0);
            var desireSize = new Size(0, 0);
            var kids = InternalChildren.Cast<UIElement>().Where(e => e.Visibility != Visibility.Collapsed);
            foreach (UIElement child in kids)
            {
                if (child.Visibility == Visibility.Collapsed)
                    continue;
                child.Measure(constraint);
                var size = (Vector)GetDesiredSizeWithoutMargin(child);
                totalSize += size;
                desireSize.Width = Math.Max(desireSize.Width, size.X); desireSize.Height = Math.Max(desireSize.Height, size.Y);
            }
            bool IsAutoSize(double val) => double.IsNaN(val) || double.IsInfinity(val);
            if (TabStripPlacement == Dock.Top || TabStripPlacement == Dock.Bottom)
                desireSize.Width = IsAutoSize(constraint.Width) ? totalSize.X : constraint.Width;
            else
                desireSize.Height = IsAutoSize(constraint.Height) ? totalSize.Y : constraint.Height;
            return desireSize;
        }

        private static (int, int) GetDisplayRange(double[] lens, double avaliableLen, int selectedIdx)
        {
            int idxPrev = selectedIdx, idxNext = selectedIdx + 1;
            bool towardsNext = false;
            while (true)
            {
                if (!towardsNext && idxPrev >= 0)
                {
                    avaliableLen -= lens[idxPrev];
                    if (avaliableLen < 0)
                        break;
                    idxPrev--;
                    if (idxNext < lens.Length)
                        towardsNext = true;
                    continue;
                }
                if (towardsNext && idxNext < lens.Length)
                {
                    avaliableLen -= lens[idxNext];
                    if (avaliableLen < 0)
                        break;
                    idxNext++;
                    if (idxPrev >= 0)
                        towardsNext = false;
                    continue;
                }
                break;
            }
            return (idxPrev, idxNext);
        }
        private static readonly Rect ZeroRect = new Rect(0, 0, 0, 0);
        protected override Size ArrangeOverride(Size arrangeSize)
        {
            var kids = InternalChildren.Cast<UIElement>().Where(e => e.Visibility != Visibility.Collapsed).ToArray();
            if (kids.Length == 0)
                return arrangeSize;
            var isHorizontal = TabStripPlacement == Dock.Top || TabStripPlacement == Dock.Bottom;
            var lens = (isHorizontal ? kids.Select(e => GetDesiredSizeWithoutMargin(e).Width) : kids.Select(e => GetDesiredSizeWithoutMargin(e).Height))
                .ToArray();
            var selIdx = kids.Select((e, i) => (bool)e.GetValue(Selector.IsSelectedProperty) ? i : -1)
                .FirstOrDefault(idx => idx >= 0);

            var (idxPrev, idxNext) = GetDisplayRange(lens, isHorizontal ? arrangeSize.Width : arrangeSize.Height, selIdx);
            var finSize = new Rect(arrangeSize);
            foreach (var (i, kid, len) in kids.Select((k, i) => (i, k, lens[i])))
            {
                if (i <= idxPrev || i >= idxNext)
                {
                    kid.Arrange(ZeroRect);
                    continue;
                }
                if (isHorizontal)
                {
                    finSize.Width = len;
                    kid.Arrange(finSize);
                    finSize.X += len;
                }
                else
                {
                    finSize.Height = len;
                    kid.Arrange(finSize);
                    finSize.Y += len;
                }
            }
            return arrangeSize;
        }

        private class DragInfo
        {
            public AnyDockTabPanel Source = null;
            public AnyDockTabLabel Label = null;
            public UIElement Item = null;
            public Point StartPoint;
        }
        private static readonly DragInfo PendingDrag = new DragInfo();

        private AnyDockTabLabel GetHittedLabel(Point pos)
        {
            var label = InputHitTest(pos) as DependencyObject;
            while (label != null && !(label is AnyDockTabLabel) && label != this)
                label = VisualTreeHelper.GetParent(label);
            return label as AnyDockTabLabel;
        }

        protected override void OnPreviewMouseLeftButtonDown(MouseButtonEventArgs e)
        {
            PendingDrag.Label = GetHittedLabel(e.GetPosition(this));
            if (PendingDrag.Label != null)
            {
                PendingDrag.StartPoint = e.GetPosition(PendingDrag.Label);
                PendingDrag.Item = PendingDrag.Label.DataContext as UIElement;
                PendingDrag.Source = this;
            }
            base.OnPreviewMouseLeftButtonDown(e);
        }
        protected override void OnPreviewMouseLeftButtonUp(MouseButtonEventArgs e)
        {
            PendingDrag.Source = null;
            base.OnPreviewMouseLeftButtonUp(e);
        }
        protected override void OnMouseLeave(MouseEventArgs e)
        {
            base.OnMouseLeave(e);
            if (e.LeftButton == MouseButtonState.Pressed && PendingDrag.Source == this)
            {
                Console.WriteLine($"Drag Leave [{PendingDrag.Source}][{PendingDrag.Item}][{PendingDrag.StartPoint}]");
                PendingDrag.Source = null;
                BeginTabItemDrag(e);
                e.Handled = true;
            }
        }
        protected override void OnMouseMove(MouseEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed && PendingDrag.Source == this)
            {
                var curPos = e.GetPosition(this);
                if (GetHittedLabel(curPos)?.DataContext is UIElement item && item != PendingDrag.Item)
                {
                    Console.WriteLine($"Drag onto  [{PendingDrag.Source}][{PendingDrag.Item}][{item}]");
                    ParentDockPoint.ReorderItem(PendingDrag.Item, item);
                    e.Handled = true;
                }
            }
            base.OnMouseMove(e);
        }

        private static Point OriginPoint = new Point(0, 0);
        private void BeginTabItemDrag(MouseEventArgs e)
        {
            PendingDrag.Source = null;

            /*    @TopLeft _ _ _ __
             *    |  * StartPoint  |
             *    |_ _ _ _ _ _ _ __|
             *       @ WinPos
             *          *CurPoint
             */
            var curPoint = Mouse.GetPosition(null);
            var deltaPos = curPoint - PendingDrag.StartPoint;
            var winPos = Window.GetWindow(this).PointToScreen((Point)deltaPos);
            //Console.WriteLine($"offset[{PendingDrag.StartPoint}], cur[{curPoint}], curOff[{Mouse.GetPosition(this)}]");
            if (AnyDockManager.GetAllowDrag(PendingDrag.Item))
            {
                if (TemplatedParent is TabControl tabControl)
                    PendingDrag.StartPoint += (Vector)TranslatePoint(OriginPoint, tabControl);
                var data = new DragData(PendingDrag.Item);
                DragManager.PerformDrag(winPos, PendingDrag.StartPoint, data);
            }
        }

        public virtual void OnDragIn(DragData data, Point pos)
        {
        }

        public virtual void OnDragOut(DragData data, Point pos)
        {
        }
        public virtual void OnDragDrop(DragData data, Point pos)
        {
            var label = GetHittedLabel(pos);
            if (label?.DataContext is UIElement dst)
                ParentDockPoint.Children.Insert(ParentDockPoint.Children.IndexOf(dst), data.Element);
            else
                ParentDockPoint.Children.Add(data.Element);
        }
    }
}
