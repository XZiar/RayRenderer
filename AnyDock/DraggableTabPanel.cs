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

        private static readonly DependencyPropertyKey IsTabsOverflowPropertyKey = DependencyProperty.RegisterReadOnly(
            "IsTabsOverflow",
            typeof(bool),
            typeof(DraggableTabPanel),
            new FrameworkPropertyMetadata(false));
        public bool IsTabsOverflow
        {
            get => (bool)GetValue(IsTabsOverflowPropertyKey.DependencyProperty);
            private set => SetValue(IsTabsOverflowPropertyKey, value);
        }

        static DraggableTabPanel()
        {
            EventManager.RegisterClassHandler(typeof(DraggableTabPanel), DragManager.RecieveDragEvent, new DragManager.RecieveDragEventHandler(OnRecieveDrag));
            ResDict = new ResourceDictionary { Source = new Uri("AnyDock;component/DraggableTabPanel.res.xaml", UriKind.RelativeOrAbsolute) };
        }

        public DraggableTabPanel()
        {
        }

        private Dock TabStripPlacement => (TemplatedParent as TabControl)?.TabStripPlacement ?? Dock.Top;
        private int? SelectedIndex => (TemplatedParent as TabControl)?.SelectedIndex;

        public bool ShowAllTabs => (TemplatedParent as DraggableTabControl)?.ShowAllTabs ?? false;


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
            var totalSize = new Vector(0, 0);
            var desireSize = new Size(0, 0);
            var childConstraint = constraint;
            var kids = InternalChildren.Cast<UIElement>().Where(x => x.Visibility != Visibility.Collapsed).ToArray();
            if (ShowAllTabs)
            {
                if (TabStripPlacement == Dock.Top || TabStripPlacement == Dock.Bottom)
                    childConstraint.Width /= kids.Length;
                else
                    childConstraint.Height /= kids.Length;
            }
            foreach (var kid in kids)
            {
                kid.Measure(childConstraint);
                var size = (Vector)GetDesiredSizeWithoutMargin(kid);
                totalSize += size;
                desireSize.Width = Math.Max(desireSize.Width, size.X); desireSize.Height = Math.Max(desireSize.Height, size.Y);
            }
            bool IsAutoSize(double val) => double.IsNaN(val) || double.IsInfinity(val);
            if (TabStripPlacement == Dock.Top || TabStripPlacement == Dock.Bottom)
            {
                desireSize.Width = IsAutoSize(constraint.Width) ? totalSize.X : constraint.Width;
                IsTabsOverflow = !ShowAllTabs && totalSize.X > constraint.Width;
            }
            else
            {
                desireSize.Height = IsAutoSize(constraint.Height) ? totalSize.Y : constraint.Height;
                IsTabsOverflow = !ShowAllTabs && totalSize.Y > constraint.Height;
            }
            return desireSize;
        }

        private struct LabelZone
        {
            public Rect BoundingBox;
            public FrameworkElement Label;
        }
        private readonly List<LabelZone> DisplayedLabels = new List<LabelZone>();
        protected override Size ArrangeOverride(Size arrangeSize)
        {
            DisplayedLabels.Clear();
            var kids = InternalChildren.Cast<FrameworkElement>()
                .Where(x => 
                {
                    if (x.Visibility != Visibility.Collapsed)
                        return true;
                    x.Arrange(ZeroRect); return false;
                })
                .ToArray();
            if (kids.Length == 0)
                return arrangeSize;
            var isHorizontal = TabStripPlacement == Dock.Top || TabStripPlacement == Dock.Bottom;
            var lens = (isHorizontal ? kids.Select(e => GetDesiredSizeWithoutMargin(e).Width) : kids.Select(e => GetDesiredSizeWithoutMargin(e).Height))
                .ToArray();

            int idxPrev = -1, idxNext = kids.Length;
            double scale = 1;
            if (ShowAllTabs)
            {
                var totalLen = lens.Sum();
                scale = (isHorizontal ? arrangeSize.Width : arrangeSize.Height) / totalLen;
                if (scale > 1) scale = 1;
            }
            else
            {
                var selIdx = SelectedIndex ?? 0;
                (idxPrev, idxNext) = GetDisplayRange(lens, isHorizontal ? arrangeSize.Width : arrangeSize.Height, selIdx);
            }

            var rect = new Rect(arrangeSize);
            for (var i = 0; i < kids.Length; ++i)
            {
                var kid = kids[i];
                if (i <= idxPrev || i >= idxNext)
                {
                    kid.Arrange(ZeroRect);
                    continue;
                }
                var len = lens[i] * scale;
                if (isHorizontal)
                    rect.Width = len;
                else
                    rect.Height = len;
                DisplayedLabels.Add(new LabelZone { BoundingBox = rect, Label = kid });
                kid.Arrange(rect);
                if (isHorizontal)
                    rect.X += len;
                else
                    rect.Y += len;
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
                var data = new DragData(PendingDrag.Item, TemplatedParent as DraggableTabControl);
                DragManager.PerformDrag(winPos, PendingDrag.StartPoint, data);
            }
        }

        private static void OnRecieveDrag(UIElement sender, DragManager.RecieveDragEventArgs e)
        {
            var self = (DraggableTabPanel)sender;
            if ((self.TemplatedParent as DraggableTabControl)?.AllowDropTab ?? false)
            {
                e.RecievePoint = self;
                e.Handled = true;
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
