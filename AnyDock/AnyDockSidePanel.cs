using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Markup;
using System.Windows.Media;

namespace AnyDock
{
    [ContentProperty(nameof(Children))]
    public class AnyDockSidePanel : ContentControl
    {
        private static readonly ResourceDictionary ResDict;
        private static readonly ControlTemplate AnyDockSidePanelTemplate;
        public static readonly DependencyProperty CollapseToSideProperty = DependencyProperty.RegisterAttached(
            "CollapseToSide",
            typeof(bool),
            typeof(AnyDockSidePanel),
            new FrameworkPropertyMetadata(false, FrameworkPropertyMetadataOptions.AffectsParentMeasure | FrameworkPropertyMetadataOptions.AffectsParentArrange));
        private static readonly DependencyPropertyDescriptor CollapseToSidePropertyDescriptor =
            DependencyPropertyDescriptor.FromProperty(CollapseToSideProperty, typeof(AnyDockSidePanel));
        public static void SetCollapseToSide(UIElement element, bool value) => element.SetValue(CollapseToSideProperty, value);
        public static bool GetCollapseToSide(UIElement element) => (bool)element?.GetValue(CollapseToSideProperty);

        internal static readonly DependencyProperty TabStripPlacementProperty =
            TabControl.TabStripPlacementProperty.AddOwner(typeof(AnyDockSidePanel),
                new FrameworkPropertyMetadata(Dock.Left, FrameworkPropertyMetadataOptions.AffectsRender | FrameworkPropertyMetadataOptions.AffectsMeasure));
        internal Dock TabStripPlacement
        {
            get => (Dock)GetValue(TabStripPlacementProperty);
            set => SetValue(TabStripPlacementProperty, value);
        }
        internal ObservableCollectionEx<UIElement> HiddenChildren { get; } = new ObservableCollectionEx<UIElement>();
        internal ObservableCollectionEx<UIElement> ShownChildren { get; } = new ObservableCollectionEx<UIElement>();


        private DockPanel RealContent;
        private HiddenBar HiddenBar;
        private DraggableTabControl MainContent;
        private Popup TemporalPage;

        public ObservableCollectionEx<UIElement> Children { get; } = new ObservableCollectionEx<UIElement>();
        private void OnCollapseToSideChanged(object sender, EventArgs e)
        {
            var ele = (UIElement)sender;
            if ((bool)CollapseToSidePropertyDescriptor.GetValue(ele))
            {
                ShownChildren.Remove(ele);
                if (!HiddenChildren.Contains(ele))
                    HiddenChildren.Add(ele);
            }
            else
            {
                TemporalPage.IsOpen = false;
                HiddenChildren.Remove(ele);
                if (!ShownChildren.Contains(ele))
                {
                    ShownChildren.Add(ele);
                    MainContent.SelectedItem = ele;
                }
            }
            return;
        }

        static AnyDockSidePanel()
        {
            ResDict = new ResourceDictionary { Source = new Uri("AnyDock;component/AnyDockSidePanel.res.xaml", UriKind.RelativeOrAbsolute) };
            AnyDockSidePanelTemplate = (ControlTemplate)ResDict["AnyDockSidePanelTemplate"];
        }
        public AnyDockSidePanel()
        {
            Children.CollectionChanged += ChildChanged;
            Template = AnyDockSidePanelTemplate;
            ApplyTemplate();
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();
            RealContent = (DockPanel)Template.FindName("RealContent", this);
            HiddenBar = (HiddenBar)Template.FindName("HiddenBar", this);
            MainContent = (DraggableTabControl)Template.FindName("MainContent", this);
            TemporalPage = (Popup)Template.FindName("PopupPage", this);
            var ct = (DroppableContentControl)Template.FindName("PopupContent", this);
            HiddenBar.ItemsSource = HiddenChildren;
            HiddenBar.ItemClicked += CollapsedItemClicked;
            MainContent.ItemsSource = ShownChildren;
            TemporalPage.Closed += OnTemporalPageClosed;
        }

        private void ChildChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            foreach (var x in e.DeledItems<UIElement>())
            {
                CollapseToSidePropertyDescriptor.RemoveValueChanged(x, OnCollapseToSideChanged);
                ((bool)x.GetValue(CollapseToSideProperty) ? HiddenChildren : ShownChildren).Remove(x);
                AnyDockManager.RemoveRemovedHandler(x, OnTabClosed);
            }
            foreach (var x in e.AddedItems<UIElement>())
            {
                CollapseToSidePropertyDescriptor.AddValueChanged(x, OnCollapseToSideChanged);
                ((bool)x.GetValue(CollapseToSideProperty) ? HiddenChildren : ShownChildren).Add(x);
                AnyDockManager.AddRemovedHandler(x, OnTabClosed);
            }
        }

        private void OnTabClosed(object sender, RoutedEventArgs args)
        {
            TemporalPage.IsOpen = false;
            Children.Remove((UIElement)sender);
        }


        private void CollapsedItemClicked(HiddenBar bar, UIElement element)
        {
            TemporalPage.DataContext = element;
            TemporalPage.IsOpen = true;
            //TemporalPage.Focus();
        }

        private void OnTemporalPageClosed(object sender, EventArgs e)
        {
            ((Popup)sender).DataContext = null;
        }
    }
}
