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
        public static readonly DependencyProperty ReferenceElementProperty = DependencyProperty.Register(
            "ReferenceElement",
            typeof(FrameworkElement),
            typeof(SquareButton),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsMeasure, OnReferenceChanged));
        public FrameworkElement ReferenceElement
        {
            get => (FrameworkElement)GetValue(ReferenceElementProperty);
            set => SetValue(ReferenceElementProperty, value);
        }

        public enum ReferenceTarget { Width, Height }
        public ReferenceTarget Target { get; set; } = ReferenceTarget.Height;

        private static void OnReferenceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((SquareButton)d).OnReferenceElementChanged((FrameworkElement)e.OldValue, (FrameworkElement)e.NewValue);
        }

        private void OnReferenceElementChanged(FrameworkElement oldVal, FrameworkElement newVal)
        {
            if (oldVal != null)
                oldVal.SizeChanged -= OnReferenceSizeChanged;
            if (newVal != null)
            {
                newVal.SizeChanged += OnReferenceSizeChanged;
                ReferenceMinSize = Target == ReferenceTarget.Width ? newVal.ActualWidth : newVal.ActualHeight;
                Width = Height = ReferenceMinSize;
            }
            else
            {
                ReferenceMinSize = double.PositiveInfinity;
                Width = Height = double.NaN;
            }
        }
        private void OnReferenceSizeChanged(object sender, SizeChangedEventArgs e)
        {
            var newMinSize = Target == ReferenceTarget.Width ?
                Math.Min(e.NewSize.Width, ReferenceElement.DesiredSize.Width) :
                Math.Min(e.NewSize.Height, ReferenceElement.DesiredSize.Height);
            if (newMinSize != ReferenceMinSize)
            {
                ReferenceMinSize = newMinSize;
                Width = Height = ReferenceMinSize;
            }
        }
        private double ReferenceMinSize = double.PositiveInfinity;

        public SquareButton()
        {
        }

    }
}
