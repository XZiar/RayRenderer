using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
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
    internal partial class HiddenBar : ItemsControl
    {
        internal delegate void ItemClickEventHandler(HiddenBar bar, UIElement element);
        internal event ItemClickEventHandler ItemClicked;
        private Border ClickedItem = null;

        public HiddenBar()
        {
            InitializeComponent();
        }

        protected override bool IsItemItsOwnContainerOverride(object item)
        {
            return false;
        }

        private void ItemMouseLBDown(object sender, MouseButtonEventArgs e)
        {
            var target = (Border)sender;
            ClickedItem = target;
            e.Handled = true;
        }

        private void ItemMouseLeave(object sender, MouseEventArgs e)
        {
            ClickedItem = null;
        }

        private void ItemMouseLBUp(object sender, MouseButtonEventArgs e)
        {
            var target = (Border)sender;
            if (ClickedItem == target)
            {
                ClickedItem = null;
                var element = (UIElement)target.DataContext;
                //Console.WriteLine($"Click Item [{element}]");
                ItemClicked(this, element);
                e.Handled = true;
            }
        }
    }
}
