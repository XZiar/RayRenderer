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
    internal class DraggableTabPanel : Panel, IDragRecievePoint
    {

        private static readonly ResourceDictionary ResDict;

        static DraggableTabPanel()
        {
            ResDict = new ResourceDictionary { Source = new Uri("AnyDock;component/DraggableTabPanel.res.xaml", UriKind.RelativeOrAbsolute) };
        }

        private readonly SquareButton MoreTabsButton;
        public DraggableTabPanel()
        {
            MoreTabsButton = ResDict["MoreTabDrop"] as SquareButton;
        }

        private Dock TabStripPlacement => (TemplatedParent as TabControl)?.TabStripPlacement ?? Dock.Top;
        private int SelectedIndex => (TemplatedParent as TabControl)?.SelectedIndex ?? -1;

        #region Layout Helper Functions
        private static Size GetDesiredSizeWithoutMargin(UIElement element)
        {
            var margin = (Thickness)element.GetValue(MarginProperty);
            var height = Math.Max(0d, element.DesiredSize.Height - margin.Top - margin.Bottom);
            var width = Math.Max(0d, element.DesiredSize.Width - margin.Left - margin.Right);
            return new Size(width, height);
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
        #endregion Layout Helper Functions

        #region Layout Functions
        protected override Size MeasureOverride(Size constraint)
        {
            //var old = base.MeasureOverride(constraint);
            var totalSize = new Vector(0, 0);
            var desireSize = new Size(0, 0);
            var kids = InternalChildren.Cast<FrameworkElement>().Where(e => e.Visibility != Visibility.Collapsed);
            foreach (UIElement child in kids)
            {
                if (child == MoreTabsButton || child.Visibility == Visibility.Collapsed)
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
            MoreTabsButton.Measure(desireSize);
            return desireSize;
        }
        //protected override void OnRender(DrawingContext dc)
        //{
        //    base.OnRender(dc);
        //    dc.
        //    MoreTabsButton.Render
        //}
        private struct LabelZone
        {
            public Rect BoundingBox;
            public FrameworkElement Label;
        }
        private readonly List<LabelZone> DisplayedLabels = new List<LabelZone>();
        protected override Size ArrangeOverride(Size arrangeSize)
        {
            DisplayedLabels.Clear();
            var kids = InternalChildren.Cast<FrameworkElement>().Where(e => e.Visibility != Visibility.Collapsed && e != MoreTabsButton).ToArray();
            if (kids.Length == 0)
                return arrangeSize;
            var isHorizontal = TabStripPlacement == Dock.Top || TabStripPlacement == Dock.Bottom;
            var lens = (isHorizontal ? kids.Select(e => GetDesiredSizeWithoutMargin(e).Width) : kids.Select(e => GetDesiredSizeWithoutMargin(e).Height))
                .ToArray();
            var selIdx = kids.Select((e, i) => (bool)e.GetValue(Selector.IsSelectedProperty) ? i : -1)
                .FirstOrDefault(idx => idx >= 0);

            var finRect = new Rect(arrangeSize);
            var (idxPrev, idxNext) = GetDisplayRange(lens, isHorizontal ? arrangeSize.Width : arrangeSize.Height, selIdx);
            //if (idxPrev >= 0 || idxNext <= kids.Length)
            //{
            //    var tlPoint = (Point)arrangeSize - (Point)MoreTabsButton.DesiredSize;
            //    var btnRect = new Rect((Point)tlPoint, (Point)arrangeSize);
            //    if (isHorizontal)
            //        finRect.Width  -= MoreTabsButton.DesiredSize.Width;
            //    else
            //        finRect.Height -= MoreTabsButton.DesiredSize.Height;
            //    MoreTabsButton.Arrange(btnRect);
            //    (idxPrev, idxNext) = GetDisplayRange(lens, isHorizontal ? finRect.Width : finRect.Height, selIdx);
            //}

            foreach (var (i, kid, len) in kids.Select((k, i) => (i, k, lens[i])))
            {
                if (i <= idxPrev || i >= idxNext)
                {
                    kid.Arrange(ZeroRect);
                    continue;
                }
                if (isHorizontal)
                {
                    finRect.Width = len;
                    DisplayedLabels.Add(new LabelZone { BoundingBox = finRect, Label = kid });
                    kid.Arrange(finRect);
                    finRect.X += len;
                }
                else
                {
                    finRect.Height = len;
                    DisplayedLabels.Add(new LabelZone { BoundingBox = finRect, Label = kid });
                    kid.Arrange(finRect);
                    finRect.Y += len;
                }
            }
            return arrangeSize;
        }
        #endregion Layout Functions

        #region Drag&Drop Functions
        private class DragInfo
        {
            public DraggableTabPanel Source = null;
            public UIElement Item = null;
            public Point StartPoint;
        }
        private static readonly DragInfo PendingDrag = new DragInfo();

        private FrameworkElement GetHittedLabel(Point pos)
        {
            foreach (var label in DisplayedLabels)
                if (label.BoundingBox.Contains(pos))
                    return label.Label;
            return null;
        }

        protected override void OnPreviewMouseLeftButtonDown(MouseButtonEventArgs e)
        {
            var label = GetHittedLabel(e.GetPosition(this));
            if (label != null)
            {
                PendingDrag.StartPoint = e.GetPosition(label);
                PendingDrag.Item = label.DataContext as UIElement;
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
                    (TemplatedParent as DraggableTabControl).ReorderItem(PendingDrag.Item, item);
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

        public virtual bool RecieveDrag()
        {
            return (TemplatedParent as DraggableTabControl)?.AllowDropTab ?? true;
        }
        public virtual void OnDragIn(DragData data, Point pos)
        {
        }

        public virtual void OnDragOut(DragData data, Point pos)
        {
        }
        public virtual void OnDragDrop(DragData data, Point pos)
        {
            var parent = TemplatedParent as DraggableTabControl;
            var label = GetHittedLabel(pos);
            if (label?.DataContext is UIElement dst)
                parent.RealChildren.Insert(parent.RealChildren.IndexOf(dst), data.Element);
            else
                parent.RealChildren.Add(data.Element);
        }
        #endregion Drag&Drop Functions

    }

}
