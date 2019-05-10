using System;
using System.Collections;
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
using System.Windows.Shapes;

namespace AnyDock
{
    internal class ItemsControl2 : ItemsControl
    {
        protected override bool IsItemItsOwnContainerOverride(object item)
        {
            return false;
        }
    }

    /// <summary>
    /// MoreTabsPopup.xaml 的交互逻辑
    /// </summary>
    public partial class MoreTabsPopup : Popup
    {
        private UIElement ClickedElement = null;
        private TaskCompletionSource<UIElement> CurrentTask = new TaskCompletionSource<UIElement>();
        
        private MoreTabsPopup(Control source, UIElement position, IEnumerable tabs)
        {
            InitializeComponent();
            PlacementTarget = position;
            MainItems.Background = source.Background; MainItems.Foreground = source.Foreground;
            MainItems.ItemsSource = tabs;
            IsOpen = true;
        }

        internal static Task<UIElement> ShowTabs(Control source, UIElement position, IEnumerable tabs)
        {
            var popup = new MoreTabsPopup(source, position, tabs);
            return popup.CurrentTask.Task;
        }

        private void ItemMouseDown(object sender, MouseButtonEventArgs e)
        {
            ClickedElement = (sender as FrameworkElement).DataContext as UIElement;
            IsOpen = false;
        }

        protected override void OnClosed(EventArgs e)
        {
            base.OnClosed(e);
            CurrentTask.SetResult(ClickedElement);
            MainItems.ItemsSource = null;
            ClickedElement = null;
            CurrentTask = null;
        }

    }
}
