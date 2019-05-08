using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Markup;
using System.Windows.Media;

namespace AnyDock
{
    [ContentProperty(nameof(Center))]
    public class AnyDockHost : Panel
    {
        private static readonly ResourceDictionary ResDict;
        private static readonly ControlTemplate AnyDockHostTemplate;

        //public static readonly DependencyProperty CenterProperty = DependencyProperty.Register(
        //    "Center",
        //    typeof(FrameworkElement),
        //    typeof(AnyDockHost),
        //    new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsMeasure));
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
            self.LeftPanel.Foreground = self.RightPanel.Foreground = self.TopPanel.Foreground = self.BottomPanel.Foreground = (Brush)e.NewValue;
        }
        private static void OnBackgroundChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var self = (AnyDockHost)d;
            self.LeftPanel.Background = self.RightPanel.Background = self.TopPanel.Background = self.BottomPanel.Background = (Brush)e.NewValue;
        }

        private AnyDockSidePanel LeftPanel, RightPanel, TopPanel, BottomPanel;
        private ContentPresenter CenterPanel;
        private FrameworkElement Center_;

        public ObservableCollectionEx<UIElement> Left   => LeftPanel.Children;
        public ObservableCollectionEx<UIElement> Right  => RightPanel.Children;
        public ObservableCollectionEx<UIElement> Top    => TopPanel.Children;
        public ObservableCollectionEx<UIElement> Bottom => BottomPanel.Children;
        public FrameworkElement Center
        {
            //get => (FrameworkElement)GetValue(CenterProperty);
            //set => SetValue(CenterProperty, value);
            get => Center_;
            set => CenterPanel.Content = Center_ = value;
        }


        public double LeftSize        { set => LeftPanel.  Width     = value; }
        public double RightSize       { set => RightPanel. Width     = value; }
        public double TopSize         { set => TopPanel.   Height    = value; }
        public double BottomSize      { set => BottomPanel.Height    = value; }
        public double LeftMaxSize     { set => LeftPanel.  MaxWidth  = value; }
        public double RightMaxSize    { set => RightPanel. MaxWidth  = value; }
        public double TopMaxSize      { set => TopPanel.   MaxHeight = value; }
        public double BottomMaxSize   { set => BottomPanel.MaxHeight = value; }
        public double CenterMinWidth  { set => CenterPanel.MinWidth  = value; }
        public double CenterMinHeight { set => CenterPanel.MinHeight = value; }

        static AnyDockHost()
        {
            //ResDict = new ResourceDictionary { Source = new Uri("AnyDock;component/AnyDockHost.res.xaml", UriKind.RelativeOrAbsolute) };
            //AnyDockHostTemplate = (ControlTemplate)ResDict["AnyDockHostTemplate"];
            BackgroundProperty.OverrideMetadata(typeof(AnyDockHost), 
                new FrameworkPropertyMetadata(new SolidColorBrush(SystemColors.WindowColor), FrameworkPropertyMetadataOptions.AffectsRender,
                OnBackgroundChanged));
        }

        public AnyDockHost()
        {
            //Template = AnyDockHostTemplate;
            //ApplyTemplate();
            LeftPanel = new AnyDockSidePanel { TabStripPlacement = Dock.Left };
            RightPanel = new AnyDockSidePanel { TabStripPlacement = Dock.Right };
            TopPanel = new AnyDockSidePanel { TabStripPlacement = Dock.Top };
            BottomPanel = new AnyDockSidePanel { TabStripPlacement = Dock.Bottom };
            CenterPanel = new ContentPresenter();
            InternalChildren.Add(LeftPanel);
            InternalChildren.Add(RightPanel);
            InternalChildren.Add(TopPanel);
            InternalChildren.Add(BottomPanel);
            InternalChildren.Add(CenterPanel);
        }

        //public override void OnApplyTemplate()
        //{
        //    base.OnApplyTemplate();
        //    LeftPanel   = (AnyDockSidePanel)Template.FindName("LeftPanel"  , this);
        //    RightPanel  = (AnyDockSidePanel)Template.FindName("RightPanel" , this);
        //    TopPanel    = (AnyDockSidePanel)Template.FindName("TopPanel"   , this);
        //    BottomPanel = (AnyDockSidePanel)Template.FindName("BottomPanel", this);
        //    CenterPanel = (ContentPresenter)Template.FindName("CenterPanel", this);
        //}

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

            var cSize = new Size(0, 0);
            if (shouldCheckCenter)
            {
                Center.Measure(constraint);
                cSize = Center.DesiredSize;
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

            var lWidth = LeftPanel.DesiredSize.Width;
            var rWidth = RightPanel.DesiredSize.Width;
            LeftPanel.Arrange(new Rect(0, 0, lWidth, finalSize.Height));
            RightPanel.Arrange(new Rect(finalSize.Width - rWidth, 0, rWidth, finalSize.Height));

            var xOffset = lWidth + pad.Left;
            var availableWidth = Math.Max(finalSize.Width - (lWidth + rWidth + pad.Left + pad.Right), 0);
            var tHeight = TopPanel.DesiredSize.Height;
            var bHeight = BottomPanel.DesiredSize.Height;
            TopPanel.Arrange(new Rect(xOffset, 0, availableWidth, tHeight));
            BottomPanel.Arrange(new Rect(xOffset, finalSize.Height - bHeight, availableWidth, bHeight));

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
