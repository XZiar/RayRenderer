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
    /// <summary>
    /// StackPanel-like, but support bidirection arrange, with size expended by specific element
    /// </summary>
    internal class DominantBidirectionalPanel : Panel
    { 
        public static readonly DependencyProperty OrientationProperty = DependencyProperty.Register(
            "Orientation",
            typeof(Orientation),
            typeof(DominantBidirectionalPanel),
            new FrameworkPropertyMetadata(Orientation.Horizontal, FrameworkPropertyMetadataOptions.AffectsMeasure));
        public Orientation Orientation
        {
            get => (Orientation)GetValue(OrientationProperty);
            set => SetValue(OrientationProperty, value);
        }
        public static readonly DependencyProperty DominantElementProperty = DependencyProperty.Register(
            "DominantElement",
            typeof(FrameworkElement),
            typeof(DominantBidirectionalPanel),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsMeasure, OnDominantChanged));
        public FrameworkElement DominantElement
        {
            get => (FrameworkElement)GetValue(DominantElementProperty);
            set => SetValue(DominantElementProperty, value);
        }
        private static void OnDominantChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((DominantBidirectionalPanel)d).InvalidateMeasure();
        }

        public static readonly DependencyProperty IsReverseProperty = DependencyProperty.RegisterAttached(
            "IsReverse",
            typeof(bool),
            typeof(DominantBidirectionalPanel),
            new FrameworkPropertyMetadata(false, FrameworkPropertyMetadataOptions.AffectsParentMeasure));
        public bool GetIsReverse(UIElement element) => (bool)element.GetValue(IsReverseProperty);
        public void SetIsReverse(UIElement element, bool value) => element.SetValue(IsReverseProperty, value);

    }

}
