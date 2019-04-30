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
using static AnyDock.AnyDockManager;

namespace AnyDockTest
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            //new RealDockWindow().ShowDialog();
            //new DPITestWindow().Show();
            //new NewDockPanel().Show();
            InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            //Sub.TabStripPlacement = (Dock)(((int)Sub.TabStripPlacement + 1) % 4);
            //var left = adpLeft.SelfCheck(0);
            //var right = adpRight.SelfCheck(0);
            //var msg = new TextBlock { Text = left+right };
            //var box = new Window { Content = msg };
            //box.ShowDialog();
        }

        private void Label_Closing(UIElement sender, TabClosingEventArgs e)
        {
            grid1.Visibility = Visibility.Collapsed;
            e.ParentPanel.IsHidden = !e.ParentPanel.IsHidden;
            e.Handled = true;
        }
    }
}
