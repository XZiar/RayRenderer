using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace AnyDock
{
    /// <summary>
    /// DockPane.xaml 的交互逻辑
    /// </summary>
    [ContentProperty(nameof(Children))]
    public partial class AnyDockPanel : ContentControl
    {
        public ObservableCollection<FrameworkElement> Children
        {
            get;
            private set;
        } = new ObservableCollection<FrameworkElement>();

        private AnyDockPanel group1, group2;
        public AnyDockPanel Group1
        {
            get { return group1; }
            set { group1 = value; OnSetGroup(Group1); }
        }
        public AnyDockPanel Group2
        {
            get { return group2; }
            set { group2 = value; OnSetGroup(Group2); }
        }
        private void OnSetGroup(AnyDockPanel group)
        {
            if (group != null)
                group.ParentPanel = this;
            if (ShouldRefresh)
            {
                RefreshState();
                RefreshRealLayout();
            }
        }
        private AnyDockPanel ParentPanel = null;

        private enum DockStates { Empty, Single, Tab, Group, Abandon };
        private DockStates State = DockStates.Single;
        private Orientation PanelOrientation = Orientation.Horizontal;
        private GridSplitter Splitter = new GridSplitter();

        public AnyDockPanel()
        {
            InitializeComponent();
            Loaded += OnLoaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            Children.CollectionChanged += new NotifyCollectionChangedEventHandler(OnChildrenChanged);
            RefreshState();
            RefreshRealLayout();
        }

        private bool ShouldRefresh = true;
        private void OnChildrenChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            var kids = (ObservableCollection<FrameworkElement>)sender;
            if(e.NewItems != null && e.NewItems.Cast<object>().Any(x => x is AnyDockPanel || x is AnyDockPage))
                throw new InvalidOperationException("DockPanel and DockPage should not be children!");
            if (ShouldRefresh)
            {
                RefreshState();
                RefreshRealLayout();
            }
        }
        private void MoveChildrenFrom(AnyDockPanel panel)
        {
            ShouldRefresh = false;
            Children.Clear();
            foreach (var ele in panel.Children)
                Children.Add(ele);
            ShouldRefresh = true;
            panel.ShouldRefresh = false;
            panel.Children.Clear();
            panel.ShouldRefresh = true;
        }

        private void RefreshState()
        {
            if (Group1 != null && Group2 != null)
            {
                State = DockStates.Group;
                if (Children.Count > 0)
                    throw new InvalidOperationException("DockPanel should not has children when group occupied!");
                AllowDrop = false;
            }
            else if (State != DockStates.Abandon)
            {
                if (Group1 != null || Group2 != null)
                    ;// throw new InvalidOperationException("Both group need to have element!");
                if (Children.Count == 0 && ParentPanel != null)
                {
                    ParentPanel.RequestRemove(this);
                    State = DockStates.Abandon;
                }
                else
                {
                    State = Children.Count > 1 ? DockStates.Tab : DockStates.Single;
                    AllowDrop = true;
                }
            }
        }
        private void RefreshRealLayout()
        {
            grid.Children.Clear();
            var dockPages = Children.Select(child => new AnyDockPage(this) { Content = child });
            switch (State)
            {
            case DockStates.Group:
                if (PanelOrientation == Orientation.Horizontal)
                {
                    Grid.SetRow(Group1, 0); Grid.SetRowSpan(Group1, 3); Grid.SetColumn(Group1, 0); Grid.SetColumnSpan(Group1, 1);
                    Grid.SetRow(Group2, 0); Grid.SetRowSpan(Group2, 3); Grid.SetColumn(Group2, 2); Grid.SetColumnSpan(Group2, 1);
                    Grid.SetRow(Splitter, 0); Grid.SetRowSpan(Splitter, 3); Grid.SetColumn(Splitter, 1); Grid.SetColumnSpan(Splitter, 1);
                    Splitter.HorizontalAlignment = HorizontalAlignment.Stretch;
                }
                else
                {
                    Grid.SetRow(Group1, 0); Grid.SetRowSpan(Group1, 1); Grid.SetColumn(Group1, 0); Grid.SetColumnSpan(Group1, 3);
                    Grid.SetRow(Group2, 2); Grid.SetRowSpan(Group2, 1); Grid.SetColumn(Group2, 0); Grid.SetColumnSpan(Group2, 3);
                    Grid.SetRow(Splitter, 1); Grid.SetRowSpan(Splitter, 1); Grid.SetColumn(Splitter, 0); Grid.SetColumnSpan(Splitter, 3);
                    Splitter.VerticalAlignment = VerticalAlignment.Stretch;
                }
                grid.Children.Add(Group1);
                grid.Children.Add(Group2);
                grid.Children.Add(Splitter);
                break;
            case DockStates.Single:
                if (dockPages.Count() > 0) 
                {
                    var page = dockPages.First();
                    Grid.SetRow(page, 0); Grid.SetRowSpan(page, 3);
                    Grid.SetColumn(page, 0); Grid.SetColumnSpan(page, 3);
                    grid.Children.Add(page);
                }
                break;
            case DockStates.Tab:
                var tabs = new TabControl();
                foreach (var page in dockPages)
                {
                    var item = new TabItem() { Content = page };
                    item.SetBinding(TabItem.HeaderProperty, 
                        new Binding { Source = page, Path = new PropertyPath(AnyDockPage.HeaderProperty), Mode = BindingMode.OneWay });
                    tabs.Items.Add(item);
                }
                Grid.SetRow(tabs, 0); Grid.SetRowSpan(tabs, 3);
                Grid.SetColumn(tabs, 0); Grid.SetColumnSpan(tabs, 3);
                grid.Children.Add(tabs);
                break;
            }
            
        }

        private void RequestRemove(AnyDockPage page)
        {
            if (State == DockStates.Group)
                throw new InvalidOperationException("Won't hold a page when in GROUP mode");
            if (!Children.Remove(page.Content as FrameworkElement))
                throw new InvalidOperationException("Doesn't hold the page");
            RefreshState();
            RefreshRealLayout();
        }
        private void RequestRemove(AnyDockPanel panel)
        {
            if (State != DockStates.Group)
                throw new InvalidOperationException("Won't hold a panel when not in GROUP mode");
            AnyDockPanel remain;
            if (Group1 == panel)
                remain = Group2;
            else if (Group2 == panel)
                remain = Group1;
            else
                throw new InvalidOperationException("Doesn't hold the panel");
            if (remain.State == DockStates.Group)
                remain.grid.Children.Clear();
            else
                MoveChildrenFrom(remain);
            ShouldRefresh = false;
            Group1 = remain.Group1;
            Group2 = remain.Group2;
            PanelOrientation = remain.PanelOrientation;
            ShouldRefresh = true;
            remain.State = DockStates.Abandon;
            RefreshState();
            RefreshRealLayout();
        }
        private void AddPage(AnyDockPage page, ModifierKeys keys)
        {
            if (keys.HasFlag(ModifierKeys.Control)) // To Right
            {
                Group1 = new AnyDockPanel();
                Group1.MoveChildrenFrom(this);
                Group2 = new AnyDockPanel();
                Group2.Children.Add(page.Content as FrameworkElement);
                PanelOrientation = Orientation.Horizontal;
                Group1.RefreshState(); Group1.RefreshRealLayout();
                Group2.RefreshState(); Group2.RefreshRealLayout();
            }
            else if (keys.HasFlag(ModifierKeys.Shift)) // To Down
            {
                Group1 = new AnyDockPanel();
                Group1.MoveChildrenFrom(this);
                Group2 = new AnyDockPanel();
                Group2.Children.Add(page.Content as FrameworkElement);
                PanelOrientation = Orientation.Vertical;
                Group1.RefreshState(); Group1.RefreshRealLayout();
                Group2.RefreshState(); Group2.RefreshRealLayout();
            }
            else
            {
                ShouldRefresh = false;
                Children.Add(page.Content as FrameworkElement);
                ShouldRefresh = true;
            }
            RefreshState();
            RefreshRealLayout();
        }
        protected override void OnDrop(DragEventArgs e)
        {
            base.OnDrop(e);
            if (State == DockStates.Group)
                return;
            if (!(e.Data.GetData("DockPage") is AnyDockPage page))
                return;
            if (!(e.Data.GetData("Keys") is ModifierKeys keys))
                return;
            page.ParentPanel.RequestRemove(page);
            if (State == DockStates.Abandon)
                ParentPanel.AddPage(page, keys);
            else
                AddPage(page, keys);
            e.Handled = true;
        }

    }
}
