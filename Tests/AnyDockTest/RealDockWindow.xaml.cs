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
using System.Windows.Shapes;

namespace AnyDockTest
{
    /// <summary>
    /// RealDockWindow.xaml 的交互逻辑
    /// </summary>
    public partial class RealDockWindow : Window
    {
        public RealDockWindow()
        {
            InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            var btn = new Button{Content="4"};
            Grid.SetColumn(btn, theGrid.ColumnDefinitions.Count);
            theGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            theGrid.Children.Add(btn);
            theGrid.UpdateLayout();
        }
    }
}
