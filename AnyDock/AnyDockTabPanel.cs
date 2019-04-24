using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

namespace AnyDock
{
    class AnyDockTabPanel : TabPanel
    {
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
    }
}
