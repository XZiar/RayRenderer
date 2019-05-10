using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;

namespace AnyDock
{
    [ContentProperty(nameof(Center))]
    public class AnyDockHost : Panel
    {
        //private static readonly ResourceDictionary ResDict;

        public static readonly DependencyProperty PaddingProperty = Control.PaddingProperty.AddOwner(
            typeof(AnyDockHost), 
            new FrameworkPropertyMetadata(new Thickness(4), FrameworkPropertyMetadataOptions.AffectsMeasure));
        public Thickness Padding
        {
            get => (Thickness)GetValue(PaddingProperty);
            set => SetValue(PaddingProperty, value);
        }

        public static readonly DependencyProperty ForegroundProperty = Control.ForegroundProperty.AddOwner(
            typeof(AnyDockHost), 
            new FrameworkPropertyMetadata(new SolidColorBrush(Colors.Black), FrameworkPropertyMetadataOptions.AffectsRender, OnForegroundChanged));
        public Brush Foreground
        {
            get => (Brush)GetValue(ForegroundProperty);
            set => SetValue(ForegroundProperty, value);
        }

        private static void OnForegroundChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var self = (AnyDockHost)d;
            self.LeftPanel.Foreground = self.RightPanel.Foreground = self.TopPanel.Foreground = self.BottomPanel.Foreground = 
                (Brush)e.NewValue;
        }
        private static void OnBackgroundChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var self = (AnyDockHost)d;
            self.LeftPanel.Background = self.RightPanel.Background = self.TopPanel.Background = self.BottomPanel.Background =
                self.LeftThumb.Background = self.RightThumb.Background = self.TopThumb.Background = self.BottomThumb.Background =
                (Brush)e.NewValue;
        }

        private readonly AnyDockSidePanel LeftPanel, RightPanel, TopPanel, BottomPanel;
        private readonly ResizeThumb LeftThumb, RightThumb, TopThumb, BottomThumb;
        private readonly ContentPresenter CenterPanel;
        private readonly PreviewResizeAdorner ResizePreview;
        private FrameworkElement Center_;

        public ObservableCollectionEx<UIElement> Left   => LeftPanel.Children;
        public ObservableCollectionEx<UIElement> Right  => RightPanel.Children;
        public ObservableCollectionEx<UIElement> Top    => TopPanel.Children;
        public ObservableCollectionEx<UIElement> Bottom => BottomPanel.Children;
        public FrameworkElement Center
        {
            get => Center_;
            set => CenterPanel.Content = Center_ = value;
        }


        public double LeftSize        { get => LeftPanel.  Width;       set => LeftPanel.  Width     = value; }
        public double RightSize       { get => RightPanel. Width;       set => RightPanel. Width     = value; }
        public double TopSize         { get => TopPanel.   Height;      set => TopPanel.   Height    = value; }
        public double BottomSize      { get => BottomPanel.Height;      set => BottomPanel.Height    = value; }
        public double LeftMaxSize     { get => LeftPanel.  MaxWidth;    set => LeftPanel.  MaxWidth  = value; }
        public double RightMaxSize    { get => RightPanel. MaxWidth;    set => RightPanel. MaxWidth  = value; }
        public double TopMaxSize      { get => TopPanel.   MaxHeight;   set => TopPanel.   MaxHeight = value; }
        public double BottomMaxSize   { get => BottomPanel.MaxHeight;   set => BottomPanel.MaxHeight = value; }
        public double CenterMinWidth  { get => CenterPanel.MinWidth;    set => CenterPanel.MinWidth  = value; }
        public double CenterMinHeight { get => CenterPanel.MinHeight;   set => CenterPanel.MinHeight = value; }

        private class PreviewResizeAdorner : Adorner
        {
            private readonly AnyDockHost Host;
            internal Rect GuideRect;
            public PreviewResizeAdorner(AnyDockHost host) : base(host)
            {
                Host = host;
            }
            protected override void OnRender(DrawingContext dc)
            {
                base.OnRender(dc);
                //Console.WriteLine($"Now Reize Rect : {GuideRect}");
                dc.DrawRectangle(Host.Foreground, null, GuideRect);
            }
        }

        static AnyDockHost()
        {
            //ResDict = new ResourceDictionary { Source = new Uri("AnyDock;component/AnyDockHost.res.xaml", UriKind.RelativeOrAbsolute) };
            BackgroundProperty.OverrideMetadata(typeof(AnyDockHost), 
                new FrameworkPropertyMetadata(new SolidColorBrush(SystemColors.WindowColor), FrameworkPropertyMetadataOptions.AffectsRender,
                OnBackgroundChanged));
        }

        public AnyDockHost()
        {
            LeftPanel   = new AnyDockSidePanel { TabStripPlacement = Dock.Left };
            RightPanel  = new AnyDockSidePanel { TabStripPlacement = Dock.Right };
            TopPanel    = new AnyDockSidePanel { TabStripPlacement = Dock.Top };
            BottomPanel = new AnyDockSidePanel { TabStripPlacement = Dock.Bottom };
            LeftThumb   = new ResizeThumb      { Cursor = Cursors.SizeWE };
            RightThumb  = new ResizeThumb      { Cursor = Cursors.SizeWE };
            TopThumb    = new ResizeThumb      { Cursor = Cursors.SizeNS };
            BottomThumb = new ResizeThumb      { Cursor = Cursors.SizeNS };
            CenterPanel = new ContentPresenter { HorizontalAlignment = HorizontalAlignment.Stretch, VerticalAlignment = VerticalAlignment.Stretch };
            ResizePreview = new PreviewResizeAdorner(this);

            InternalChildren.Add(LeftPanel);
            InternalChildren.Add(RightPanel);
            InternalChildren.Add(TopPanel);
            InternalChildren.Add(BottomPanel);
            InternalChildren.Add(LeftThumb);
            InternalChildren.Add(RightThumb);
            InternalChildren.Add(TopThumb);
            InternalChildren.Add(BottomThumb);
            InternalChildren.Add(CenterPanel);

            void AddDragHandler(ResizeThumb thumb)
            {
                thumb.DragStarted += ResizeThumbStarted;
                thumb.DragDelta += ResizeThumbDelta;
                thumb.DragCompleted += ResizeThumbCompleted;
            }
            AddDragHandler(LeftThumb);
            AddDragHandler(RightThumb);
            AddDragHandler(TopThumb);
            AddDragHandler(BottomThumb);
        }


        private Thickness SizeOffsets;
        private bool IsInReSize = false;
        private void ResizeThumbStarted(object sender, DragStartedEventArgs e)
        {
            if (sender == LeftThumb)
            {
                if (LeftPanel.Children.Count > 0)
                    ResizePreview.GuideRect = new Rect(SizeOffsets.Left, 0, Padding.Left, ActualHeight);
                else return;
            }
            else if (sender == RightThumb)
            {
                if (RightPanel.Children.Count > 0)
                    ResizePreview.GuideRect = new Rect(SizeOffsets.Right, 0, Padding.Right, ActualHeight);
                else return;
            }
            else if (sender == TopThumb)
            {
                if (TopPanel.Children.Count > 0)
                    ResizePreview.GuideRect = new Rect(0, SizeOffsets.Top, ActualWidth, Padding.Top);
                else return;
            }
            else if (sender == BottomThumb)
            {
                if (BottomPanel.Children.Count > 0)
                    ResizePreview.GuideRect = new Rect(0, SizeOffsets.Bottom, ActualWidth, Padding.Bottom);
                else return;
            }
            else
                return;
            IsInReSize = true;
            AdornerLayer.GetAdornerLayer(this).Add(ResizePreview);
        }
        private void ResizeThumbDelta(object sender, DragDeltaEventArgs e)
        {
            double Clamp(double input, double min, double max, double padding)
            {
                min += padding; max -= padding;
                if (input < min) return min;
                if (input > max) return max;
                return input;
            }
            if (!IsInReSize) return;
            if (sender == LeftThumb)
                ResizePreview.GuideRect.X = Clamp(SizeOffsets.Left + e.HorizontalChange,  0, ActualWidth - RightPanel.ActualWidth, 30);
            else if (sender == RightThumb)
                ResizePreview.GuideRect.X = Clamp(SizeOffsets.Right + e.HorizontalChange, LeftPanel.ActualWidth, ActualWidth, 30);
            else if (sender == TopThumb)
                ResizePreview.GuideRect.Y = Clamp(SizeOffsets.Top + e.VerticalChange,     0, ActualHeight - BottomPanel.ActualHeight, 30);
            else if (sender == BottomThumb)
                ResizePreview.GuideRect.Y = Clamp(SizeOffsets.Bottom + e.VerticalChange,  TopPanel.ActualHeight, ActualHeight, 30);
            else
                return;
            ResizePreview.InvalidateVisual();
        }
        private void ResizeThumbCompleted(object sender, DragCompletedEventArgs e)
        {
            if (IsInReSize)
            {
                IsInReSize = false;
                AdornerLayer.GetAdornerLayer(this).Remove(ResizePreview);
                if (sender == LeftThumb)
                {
                    LeftMaxSize = ResizePreview.GuideRect.X;
                    RightMaxSize = Math.Min(RightMaxSize, RightPanel.ActualWidth);
                }
                else if (sender == RightThumb)
                {
                    RightMaxSize = ActualWidth - ResizePreview.GuideRect.X;
                    LeftMaxSize = Math.Min(LeftMaxSize, LeftPanel.ActualWidth);
                }
                else if (sender == TopThumb)
                {
                    TopMaxSize = ResizePreview.GuideRect.Y;
                    BottomMaxSize = Math.Min(BottomMaxSize, BottomPanel.ActualHeight);
                }
                else if (sender == BottomThumb)
                {
                    BottomMaxSize = ActualHeight - ResizePreview.GuideRect.Y;
                    TopMaxSize = Math.Min(TopMaxSize, TopPanel.ActualHeight);
                }
                else
                    return;
                InvalidateMeasure();
            }
        }

        private static bool IsAutoSize(double val) => double.IsNaN(val) || double.IsInfinity(val);
        protected override Size MeasureOverride(Size availableSize)
        {
            var shouldCheckCenter = Center != null && Center.Visibility != Visibility.Collapsed;
            var pad = Padding;
            var padSize = new Size(pad.Left + pad.Right, pad.Top + pad.Bottom);
            var minSize = new Vector(0, 0);
            if (shouldCheckCenter)
            {
                minSize.X = IsAutoSize(CenterPanel.MinWidth) ? 0 : CenterPanel.MinWidth;
                minSize.Y = IsAutoSize(CenterPanel.MinHeight) ? 0 : CenterPanel.MinHeight;
            }
            var constraint = availableSize;

            // measure Left & Right, if exceed availableSize, scale them proportionally
            constraint.Width -= Math.Min(padSize.Width + minSize.X, constraint.Width);
            LeftPanel.Measure(constraint);
            RightPanel.Measure(constraint);
            var lrWidth = LeftPanel.DesiredSize.Width + RightPanel.DesiredSize.Width;
            if (lrWidth > constraint.Width) // need scale
            {
                LeftPanel.Measure(new Size(LeftPanel.DesiredSize.Width * constraint.Width / lrWidth, constraint.Height));
                RightPanel.Measure(new Size(RightPanel.DesiredSize.Width * constraint.Width / lrWidth, constraint.Height));
                lrWidth = LeftPanel.DesiredSize.Width + RightPanel.DesiredSize.Width;
                constraint.Width = minSize.X;
            }
            else
                constraint.Width -= lrWidth - minSize.X;
            LeftThumb.Measure(new Size(pad.Left, constraint.Height));
            RightThumb.Measure(new Size(pad.Right, constraint.Height));

            // measure Top & Bottom, if exceed availableSize, scale them proportionally
            constraint.Height -= Math.Min(padSize.Height + minSize.Y, constraint.Height);
            TopPanel.Measure(constraint);
            BottomPanel.Measure(constraint);
            var tbHeight = TopPanel.DesiredSize.Height + BottomPanel.DesiredSize.Height;
            if (tbHeight > constraint.Height) // need scale
            {
                TopPanel.Measure(new Size(constraint.Width, TopPanel.DesiredSize.Height * constraint.Height / tbHeight));
                BottomPanel.Measure(new Size(constraint.Width, BottomPanel.DesiredSize.Height * constraint.Height / tbHeight));
                tbHeight = TopPanel.DesiredSize.Height + BottomPanel.DesiredSize.Height;
                constraint.Height = minSize.Y;
            }
            else
                constraint.Height -= tbHeight - minSize.Y;
            TopThumb.Measure(new Size(constraint.Width, pad.Top));
            BottomThumb.Measure(new Size(constraint.Width, pad.Bottom));

            var cSize = new Size(0, 0);
            if (shouldCheckCenter)
            {
                CenterPanel.Measure(constraint);
                cSize = CenterPanel.DesiredSize;
            }

            var totalWidth = lrWidth + padSize.Width +
                Math.Max(Math.Max(TopPanel.DesiredSize.Width, BottomPanel.DesiredSize.Width), cSize.Width);
            var totalHeight = Math.Max(Math.Max(LeftPanel.DesiredSize.Height, RightPanel.DesiredSize.Height), 
                tbHeight + padSize.Height + cSize.Height);

            return new Size(Math.Min(totalWidth, availableSize.Width), Math.Min(totalHeight, availableSize.Height));
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            var shouldCheckCenter = Center != null && Center.Visibility != Visibility.Collapsed;
            var pad = Padding;

            // Arrange Left & Right, record LRthumbs' location as offsets
            var lWidth = LeftPanel.DesiredSize.Width;
            var rWidth = RightPanel.DesiredSize.Width;
            LeftPanel.Arrange(new Rect(0, 0, lWidth, finalSize.Height));
            RightPanel.Arrange(new Rect(finalSize.Width - rWidth, 0, rWidth, finalSize.Height));
            SizeOffsets.Left = lWidth;
            LeftThumb.Arrange(new Rect(SizeOffsets.Left, 0, pad.Left, finalSize.Height));
            SizeOffsets.Right = finalSize.Width - rWidth - pad.Right;
            RightThumb.Arrange(new Rect(SizeOffsets.Right, 0, pad.Right, finalSize.Height));

            // Arrange Top & Down, record TBthumbs' location as offsets
            var xOffset = lWidth + pad.Left;
            var availableWidth = Math.Max(finalSize.Width - (lWidth + rWidth + pad.Left + pad.Right), 0);
            var tHeight = TopPanel.DesiredSize.Height;
            var bHeight = BottomPanel.DesiredSize.Height;
            TopPanel.Arrange(new Rect(xOffset, 0, availableWidth, tHeight));
            BottomPanel.Arrange(new Rect(xOffset, finalSize.Height - bHeight, availableWidth, bHeight));
            SizeOffsets.Top = tHeight;
            TopThumb.Arrange(new Rect(xOffset, SizeOffsets.Top, availableWidth, pad.Top));
            SizeOffsets.Bottom = finalSize.Height - bHeight - pad.Bottom;
            BottomThumb.Arrange(new Rect(xOffset, SizeOffsets.Bottom, availableWidth, pad.Bottom));

            if (!shouldCheckCenter)
                CenterPanel.Arrange(new Rect(0, 0, 0, 0));
            else
            {
                var yOffset = tHeight + pad.Top;
                var availableHeight = Math.Max(finalSize.Height - (tHeight + bHeight + pad.Top + pad.Bottom), 0);
                CenterPanel.Arrange(new Rect(xOffset, yOffset, availableWidth, availableHeight));
            }
            return finalSize;
        }


    }
}
