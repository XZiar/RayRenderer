using System;
using System.Collections.Generic;
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
    /// HiddenBar.xaml 的交互逻辑
    /// </summary>
    public partial class HiddenBar : ItemsControl
    {
        internal static readonly DependencyProperty TabStripPlacementProperty =
            TabControl.TabStripPlacementProperty.AddOwner(typeof(HiddenBar),
                new FrameworkPropertyMetadata(Dock.Right, FrameworkPropertyMetadataOptions.AffectsRender | FrameworkPropertyMetadataOptions.AffectsMeasure));
        internal Dock TabStripPlacement
        {
            get => (Dock)GetValue(TabStripPlacementProperty);
            set => SetValue(TabStripPlacementProperty, value);
        }

        public HiddenBar()
        {
            InitializeComponent();
        }

        protected override bool IsItemItsOwnContainerOverride(object item)
        {
            return false;
        }

        private void ItemMouseDown(object sender, MouseButtonEventArgs e)
        {
            var element = (UIElement)((Border)sender).DataContext;
            Console.WriteLine($"Click Item [{element}]");
        }
    }
}
