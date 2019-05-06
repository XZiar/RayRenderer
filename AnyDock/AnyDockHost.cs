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
    public class AnyDockHost : ContentControl
    {
        private static readonly ResourceDictionary ResDict;
        private static readonly ControlTemplate AnyDockHostTemplate;
        public static readonly DependencyProperty CenterProperty = DependencyProperty.Register(
            "Center",
            typeof(UIElement),
            typeof(AnyDockHost),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsMeasure));

        private AnyDockSidePanel LeftPanel, RightPanel, TopPanel, BottomPanel;
        private ContentPresenter CenterPanel;

        public ObservableCollectionEx<UIElement> Left   => LeftPanel.Children;
        public ObservableCollectionEx<UIElement> Right  => RightPanel.Children;
        public ObservableCollectionEx<UIElement> Top    => TopPanel.Children;
        public ObservableCollectionEx<UIElement> Bottom => BottomPanel.Children;
        public UIElement Center
        {
            get => (UIElement)GetValue(CenterProperty);
            set => SetValue(CenterProperty, value);
        }

        static AnyDockHost()
        {
            ResDict = new ResourceDictionary { Source = new Uri("AnyDock;component/AnyDockHost.res.xaml", UriKind.RelativeOrAbsolute) };
            AnyDockHostTemplate = (ControlTemplate)ResDict["AnyDockHostTemplate"];
            BackgroundProperty.OverrideMetadata(typeof(AnyDockHost), new FrameworkPropertyMetadata(new SolidColorBrush(SystemColors.WindowColor), FrameworkPropertyMetadataOptions.AffectsRender));
        }

        public AnyDockHost()
        {
            Template = AnyDockHostTemplate;
            ApplyTemplate();
            //LeftPanel   = new AnyDockSidePanel { TabStripPlacement = Dock.Left   };
            //RightPanel  = new AnyDockSidePanel { TabStripPlacement = Dock.Right  };
            //TopPanel    = new AnyDockSidePanel { TabStripPlacement = Dock.Top    };
            //BottomPanel = new AnyDockSidePanel { TabStripPlacement = Dock.Bottom };
            //InternalChildren.Add(LeftPanel);
            //InternalChildren.Add(RightPanel);
            //InternalChildren.Add(TopPanel);
            //InternalChildren.Add(BottomPanel);
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();
            LeftPanel   = (AnyDockSidePanel)Template.FindName("LeftPanel"  , this);
            RightPanel  = (AnyDockSidePanel)Template.FindName("RightPanel" , this);
            TopPanel    = (AnyDockSidePanel)Template.FindName("TopPanel"   , this);
            BottomPanel = (AnyDockSidePanel)Template.FindName("BottomPanel", this);
            CenterPanel = (ContentPresenter)Template.FindName("CenterPanel", this);
        }

        //protected override Size MeasureOverride(Size availableSize)
        //{
        //    var shouldCheckCenter = Center != null && Center.Visibility != Visibility.Collapsed;
        //    LeftPanel.Measure(availableSize);
        //    RightPanel.Measure(availableSize);
        //    TopPanel.Measure(availableSize);
        //    BottomPanel.Measure(availableSize);
        //    if (shouldCheckCenter)
        //        Center.Measure(availableSize);

        //    var totalWidth = LeftPanel.DesiredSize.Width + RightPanel.DesiredSize.Width +
        //        Math.Max(Math.Max(TopPanel.DesiredSize.Width, BottomPanel.DesiredSize.Width),
        //            shouldCheckCenter ? Center.DesiredSize.Width : 0);
        //    var totalHeight = Math.Max(Math.Max(LeftPanel.DesiredSize.Height, RightPanel.DesiredSize.Height),
        //            TopPanel.DesiredSize.Height + BottomPanel.DesiredSize.Height + (shouldCheckCenter ? Center.DesiredSize.Width : 0));

        //    bool IsAutoSize(double val) => double.IsNaN(val) || double.IsInfinity(val);
        //    var desiredSize = availableSize;
        //    if (!IsAutoSize(availableSize.Width))
        //    {
        //        desiredSize.Width *= (LeftPanel.DesiredSize.Width / totalWidth);
        //        LeftPanel.Measure(desiredSize);
        //        desiredSize.Width *= (RightPanel.DesiredSize.Width / totalWidth);
        //        RightPanel.Measure(desiredSize);
        //    }

        //    LeftPanel
        //    desiredSize.Width += ;
        //    desiredSize.Height = Math.Max(Math.Max(TopPanel.DesiredSize.Height, BottomPanel.DesiredSize.Height), desiredSize.Height);

        //    //if (!IsAutoSize(availableSize.Width)) desiredSize.Width = availableSize.Width;
        //    //if (!IsAutoSize(availableSize.Height)) desiredSize.Height = availableSize.Height;
        //    return desiredSize;
        //}

        //protected override Size ArrangeOverride(Size finalSize)
        //{
        //    var rect = new Rect(0, 0, 0, finalSize.Height);

        //    rect.Width = LeftPanel.DesiredSize.Width;
        //    LeftPanel.Arrange(rect);

        //    rect.Width = RightPanel.DesiredSize.Width;
        //    rect.X = finalSize.Width - rect.Width;
        //    RightPanel.Arrange(rect);

        //    rect.Width = finalSize.Width - LeftPanel.DesiredSize.Width + RightPanel.DesiredSize.Width;
        //    rect.Height = TopPanel.DesiredSize.Height;
        //    rect.X = 0;
        //    TopPanel.Arrange(rect);

        //    rect.Height = BottomPanel.DesiredSize.Height;
        //    rect.Y = finalSize.Height - rect.Height;
        //    BottomPanel.Arrange(rect);

        //    if (Center != null && Center.Visibility != Visibility.Collapsed)
        //    {
        //        rect.X = LeftPanel.DesiredSize.Width;
        //        rect.Y = TopPanel.DesiredSize.Height;
        //        rect.Size = Center.DesiredSize;
        //        Center.Arrange(rect);
        //    }
        //    return finalSize;
        //}
    }
}
