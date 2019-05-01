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
    public partial class MoreTabsPopup : ContextMenu
    {
        private UIElement ClickedElement = null;
        private TaskCompletionSource<UIElement> CurrentTask;

        internal MoreTabsPopup()
        {
            InitializeComponent();
        }

        private void ItemClick(object sender, RoutedEventArgs e)
        {
            ClickedElement = (sender as FrameworkElement).DataContext as UIElement;
        }

        internal Task<UIElement> ShowTabs(UIElement position, IEnumerable tabs)
        {
            CurrentTask = new TaskCompletionSource<UIElement>();
            PlacementTarget = position;
            ItemsSource = tabs;
            IsOpen = true;
            return CurrentTask.Task;
        }

        protected override void OnClosed(RoutedEventArgs e)
        {
            base.OnClosed(e);
            CurrentTask.SetResult(ClickedElement);
            ItemsSource = null;
            ClickedElement = null;
            CurrentTask = null;
        }

    }
}
