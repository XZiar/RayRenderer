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

namespace AnyDock
{
    /// <summary>
    /// AnyDockTabLabel.xaml 的交互逻辑
    /// </summary>
    public partial class AnyDockTabLabel : ContentControl
    {
        internal static readonly DependencyProperty ParentTabProperty = DependencyProperty.Register(
            "ParentTab",
            typeof(DraggableTabControl),
            typeof(AnyDockTabLabel),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        internal DraggableTabControl ParentTab
        {
            get => (DraggableTabControl)GetValue(ParentTabProperty);
            set => SetValue(ParentTabProperty, value);
        }

        public AnyDockTabLabel()
        {
            InitializeComponent();
        }

        private void HandleClose(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)DataContext;
            var earg = new AnyDockManager.TabCloseEventArgs(element);
            element.RaiseEvent(earg);
            if (earg.ShouldClose)
            {
                AnyDockManager.RaiseRemovedEvent(element);
            }
        }

    }
}
