using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using XZiar.Util;

namespace AnyDock
{
    /// <summary>
    /// AnyDockTabLabel.xaml 的交互逻辑
    /// </summary>
    public partial class AnyDockTabLabel : ContentControl
    {
        private AnyDockPanel ParentPanel = null;

        public static readonly DependencyProperty ParentTabProperty = DependencyProperty.Register(
            "ParentTab",
            typeof(TabControl),
            typeof(AnyDockTabLabel),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        public TabControl ParentTab { get => (TabControl)GetValue(ParentTabProperty); set => SetValue(ParentTabProperty, value); }


        public AnyDockTabLabel()
        {
            DataContextChanged += (o, e) =>
                {
                    ParentPanel = AnyDockManager.GetParentDock((UIElement)e.NewValue);
                };
            InitializeComponent();
        }

        private void HandleClose(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)DataContext;
            var earg = new AnyDockManager.TabClosingEventArgs(element, ParentPanel);
            element.RaiseEvent(earg);
            if (earg.ShouldClose)
                ParentPanel.Children.Remove(element);
        }

    }
}
