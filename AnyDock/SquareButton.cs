using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace AnyDock
{
    class SquareButton : Button
    {
        public static readonly DependencyProperty TargetSizeProperty = DependencyProperty.Register(
            "TargetSize",
            typeof(double),
            typeof(SquareButton),
            new FrameworkPropertyMetadata(double.PositiveInfinity, FrameworkPropertyMetadataOptions.AffectsMeasure));
        public double TargetSize
        {
            get => (double)GetValue(TargetSizeProperty);
            set => SetValue(TargetSizeProperty, value);
        }
        protected override Size MeasureOverride(Size constraint)
        {
            var minSize = Math.Min(Math.Min(constraint.Width, constraint.Height), TargetSize);
            return base.MeasureOverride(new Size(minSize, minSize));
        }

        protected override Size ArrangeOverride(Size arrangeBounds)
        {
            return base.ArrangeOverride(arrangeBounds);
        }
    }
}
