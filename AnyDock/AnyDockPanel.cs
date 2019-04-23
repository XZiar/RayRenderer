﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Runtime.CompilerServices;
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
using XZiar.Util;

namespace AnyDock
{
    [ContentProperty(nameof(Children))]
    public class AnyDockPanel : ContentControl
    {
        private static readonly ResourceDictionary ResDict;
        private static readonly ControlTemplate AnyDockPanelTemplate;
        private static readonly Viewbox DragOverLay;

        private readonly Grid MainPanel;
        private readonly Grid MainGrid;
        private readonly TabControl MainTab;
        private readonly GridSplitter Splitter;
        private AnyDockPanel ParentPanel = null;
        private AnyDockPanel group1, group2;

        public static readonly DependencyProperty TabStripPlacementProperty = 
            TabControl.TabStripPlacementProperty.AddOwner(typeof(AnyDockPanel),
                new FrameworkPropertyMetadata(Dock.Top, FrameworkPropertyMetadataOptions.AffectsRender));
        public Dock TabStripPlacement
        {
            get => (Dock)GetValue(TabStripPlacementProperty);
            set => SetValue(TabStripPlacementProperty, value);
        }

        public static readonly DependencyProperty IsHiddenProperty = DependencyProperty.Register(
            "IsHidden",
            typeof(bool),
            typeof(AnyDockPanel),
            new FrameworkPropertyMetadata(false, FrameworkPropertyMetadataOptions.AffectsRender));
        public bool IsHidden
        {
            get => (bool)GetValue(IsHiddenProperty);
            set => SetValue(IsHiddenProperty, value);
        }

        public static readonly DependencyProperty OrientationProperty = DependencyProperty.Register(
            "Orientation",
            typeof(Orientation),
            typeof(AnyDockPanel),
            new FrameworkPropertyMetadata(Orientation.Horizontal, FrameworkPropertyMetadataOptions.AffectsRender, OnOrientationChanged));
        private static void OnOrientationChanged(DependencyObject obj, DependencyPropertyChangedEventArgs e)
        {
            if (e.NewValue != e.OldValue) 
            {
                var self = (AnyDockPanel)obj;
                var newOri = (Orientation)e.NewValue;
                //self.MainGrid.RowDefinitions.Clear();
                //self.MainGrid.ColumnDefinitions.Clear();
                //if (newOri == Orientation.Horizontal)
                //{
                //    self.MainGrid.RowDefinitions.Add(new RowDefinition());
                //    self.MainGrid.ColumnDefinitions.Add(new ColumnDefinition { Width=new GridLength(0, GridUnitType.Auto) });
                //    self.MainGrid.ColumnDefinitions.Add(new ColumnDefinition { Width=new GridLength(0, GridUnitType.Auto) });
                //    self.MainGrid.ColumnDefinitions.Add(new ColumnDefinition { Width=new GridLength(0, GridUnitType.Auto) });
                //}
                //else
                //{

                //}
                //if (self.Group1 != null)

            }
        }

        public Orientation Orientation
        {
            get => (Orientation)GetValue(OrientationProperty);
            set => SetValue(OrientationProperty, value);
        }

        public enum AnyDockStates { Tab, Group, Abandon };
        private static readonly DependencyPropertyKey StatePropertyKey = DependencyProperty.RegisterReadOnly(
            "State",
            typeof(AnyDockStates),
            typeof(AnyDockPanel),
            new FrameworkPropertyMetadata(AnyDockStates.Tab, FrameworkPropertyMetadataOptions.AffectsRender));
        public static readonly DependencyProperty StateProperty = StatePropertyKey.DependencyProperty;
        public AnyDockStates State
        {
            get { return (AnyDockStates)GetValue(StateProperty); }
            protected set { SetValue(StatePropertyKey, value); }
        }

        public ObservableCollection<UIElement> Children { get; } = new ObservableCollection<UIElement>();

        public AnyDockPanel Group1
        {
            get { return group1; }
            set { OnSetGroup(ref group1, value); }
        }
        public AnyDockPanel Group2
        {
            get { return group2; }
            set { OnSetGroup(ref group2, value); }
        }
        private void OnSetGroup(ref AnyDockPanel dst, AnyDockPanel group)
        {
            if (dst != null)
                MainGrid.Children.Remove(dst);
            if (group != null)
            {
                group.ParentPanel = this;
                //group.Visibility = Visibility.Collapsed;
                MainGrid.Children.Add(group);
            }
            dst = group;
            if (ShouldRefresh)
                RefreshState();
        }
        private void OnChildrenChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.NewItems != null)
            {
                foreach (var x in e.NewItems.Cast<object>())
                {
                    if (!(x is UIElement ele))
                        throw new InvalidOperationException("Only UIElement can be added");
                    if (ele is AnyDockPanel)
                        throw new InvalidOperationException("DockPanel should not be children!");
                    AnyDockManager.SetParentDock(ele, this);
                }
            }
            if (e.Action == NotifyCollectionChangedAction.Remove || e.Action == NotifyCollectionChangedAction.Reset)
            {
                if (e.OldItems != null)
                    foreach (var x in e.OldItems.Cast<UIElement>())
                        AnyDockManager.SetParentDock(x, null);
                if (Children.Count == 0 && ShouldRefresh)
                    RefreshState();
            }
        }

        private bool ShouldRefresh = false;

        public bool AllowDropTab { get; set; } = true;

        static AnyDockPanel()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(AnyDockPanel), new FrameworkPropertyMetadata(typeof(AnyDockPanel)));
            ResDict = new ResourceDictionary { Source = new Uri("AnyDock;component/AnyDockPanel.res.xaml", UriKind.RelativeOrAbsolute) };
            AnyDockPanelTemplate = (ControlTemplate)ResDict["AnyDockPanelTemplate"];
            DragOverLay = (Viewbox)ResDict["DragOverLay"];
        }

        public AnyDockPanel()
        {
            Template = AnyDockPanelTemplate;
            ApplyTemplate();
            MainPanel = (Grid)Template.FindName("MainPanel", this);
            MainGrid = (Grid)Template.FindName("MainGrid", this);
            MainTab = (TabControl)Template.FindName("MainTab", this);
            Splitter = (GridSplitter)Template.FindName("Splitter", this);

            Children.CollectionChanged += new NotifyCollectionChangedEventHandler(OnChildrenChanged);
            Loaded += (o, e) => 
            {
                ShouldRefresh = true;
                RefreshState();
            };
        }


        private void RefreshState()
        {
            if (State == AnyDockStates.Abandon)
                return;
            if (Group1 != null && Group2 != null)
            {
                if (Children.Count > 0)
                    throw new InvalidOperationException("AnyDockPanel should not has children when group occupied!");
                if (Orientation == Orientation.Horizontal)
                {
                    Grid.SetRow(Group1, 0); Grid.SetRowSpan(Group1, 3); Grid.SetColumn(Group1, 0); Grid.SetColumnSpan(Group1, 1);
                    Grid.SetRow(Group2, 0); Grid.SetRowSpan(Group2, 3); Grid.SetColumn(Group2, 2); Grid.SetColumnSpan(Group2, 1);
                    Grid.SetRow(Splitter, 0); Grid.SetRowSpan(Splitter, 3); Grid.SetColumn(Splitter, 1); Grid.SetColumnSpan(Splitter, 1);
                }
                else
                {
                    Grid.SetRow(Group1, 0); Grid.SetRowSpan(Group1, 1); Grid.SetColumn(Group1, 0); Grid.SetColumnSpan(Group1, 3);
                    Grid.SetRow(Group2, 2); Grid.SetRowSpan(Group2, 1); Grid.SetColumn(Group2, 0); Grid.SetColumnSpan(Group2, 3);
                    Grid.SetRow(Splitter, 1); Grid.SetRowSpan(Splitter, 1); Grid.SetColumn(Splitter, 0); Grid.SetColumnSpan(Splitter, 3);
                }
                State = AnyDockStates.Group;
            }
            else
            {
                if (Group1 != null || Group2 != null)
                    throw new InvalidOperationException("Both group need to have element!");
                if (Children.Count == 0 && ParentPanel != null) // Children just get removed
                {
                    if (ParentPanel.State != AnyDockStates.Group)
                        throw new InvalidOperationException("Won't hold a panel when not in GROUP mode");
                    AnyDockPanel remain;
                    if (ParentPanel.Group1 == this)
                        remain = ParentPanel.Group2;
                    else if (ParentPanel.Group2 == this)
                        remain = ParentPanel.Group1;
                    else
                        throw new InvalidOperationException("Parent doesn't hold the panel");
                    remain.HandoverToParent();
                    State = AnyDockStates.Abandon;
                }
                else
                {
                    State = AnyDockStates.Tab;
                }
            }
        }
        private void MoveChildrenTo(AnyDockPanel dst)
        {
            if (dst.Children.Count != 0)
                throw new InvalidOperationException("Should not has children when being moved.");
            var sel = MainTab.SelectedIndex;
            var kids = Children.ToArray();
            ShouldRefresh = false;
            Children.Clear();
            ShouldRefresh = true;
            foreach (var ele in kids)
                dst.Children.Add(ele);
            dst.MainTab.SelectedIndex = sel;
        }
        private void HandoverToParent()
        {
            if (ParentPanel == null)
                return;
            if (ParentPanel.State != AnyDockStates.Group)
                throw new InvalidOperationException("Won't hold a panel when not in GROUP mode");
            if (State == AnyDockStates.Tab)
            {
                ParentPanel.ShouldRefresh = false;
                ParentPanel.Group1 = ParentPanel.Group2 = null;
                ParentPanel.ShouldRefresh = true;
                MoveChildrenTo(ParentPanel);
            }
            else
            {
                ParentPanel.ShouldRefresh = ShouldRefresh = false;
                AnyDockPanel gp1 = Group1, gp2 = Group2;
                Group1 = Group2 = null;
                ParentPanel.Orientation = Orientation;
                ParentPanel.TabStripPlacement = TabStripPlacement;
                ParentPanel.Group1 = gp1; ParentPanel.Group2 = gp2;
                ParentPanel.ShouldRefresh = ShouldRefresh = true;
            }
            ParentPanel.RefreshState();
            State = AnyDockStates.Abandon;
        }
        

        public void SetGroups(AnyDockPanel panel1, AnyDockPanel panel2, Orientation orientation = Orientation.Horizontal)
        {
            ShouldRefresh = false;
            Group1 = panel1; Group2 = panel2;
            Orientation = orientation;
            ShouldRefresh = true;
            RefreshState();
        }

        internal void OnContentDragEnter(AnyDockContent content)
        {
            Console.WriteLine($"Drag enter [{this.GetHashCode()}] with [{content.GetHashCode()}]({(content.Content as Label).Content})");
            DragOverLay.Height = content.ActualHeight;
            DragOverLay.Width = content.ActualWidth;
            switch(TabStripPlacement)
            {
            case Dock.Top:    DragOverLay.VerticalAlignment = VerticalAlignment.Bottom; DragOverLay.HorizontalAlignment = HorizontalAlignment.Center; break;
            case Dock.Bottom: DragOverLay.VerticalAlignment = VerticalAlignment.Top;    DragOverLay.HorizontalAlignment = HorizontalAlignment.Center; break;
            case Dock.Left:   DragOverLay.VerticalAlignment = VerticalAlignment.Center; DragOverLay.HorizontalAlignment = HorizontalAlignment.Right;  break;
            case Dock.Right:  DragOverLay.VerticalAlignment = VerticalAlignment.Center; DragOverLay.HorizontalAlignment = HorizontalAlignment.Left;   break;
            }
            MainPanel.Children.Add(DragOverLay);
        }
        internal void OnContentDragLeave(AnyDockContent content)
        {
            MainPanel.Children.Remove(DragOverLay);
        }
        internal void OnContentDrop(AnyDockContent content, DragData src, DragEventArgs e)
        {
            var hitted = VisualTreeHelper.HitTest(DragOverLay, e.GetPosition(DragOverLay));
            OnContentDragLeave(content);
            if (hitted == null || !((hitted.VisualHit as Polygon)?.Tag is string hitPart))
                return;
            if (hitPart == "Middle")
            {
                if (Children.Count == 0)
                {
                    src.Panel.Children.Remove(src.Element);
                    Children.Add(src.Element);
                }
                else
                {
                    var dst = new DragData((UIElement)content.Content);
                    DoDropTab(src, dst);
                }
            }
            else
            {
                if (Group1 != null || Group2 != null)
                    throw new InvalidOperationException("Panel should not has groups when accept drop");
                AnyDockPanel panel1 = new AnyDockPanel(), panel2 = new AnyDockPanel();
                src.Panel.Children.Remove(src.Element);
                var dstPanel = State == AnyDockStates.Abandon ? ParentPanel : this; // in case collapsed
                switch (hitPart)
                {
                case "Up":
                case "Left":
                    panel1.Children.Add(src.Element);
                    dstPanel.MoveChildrenTo(panel2);
                    break;
                case "Down":
                case "Right":
                    dstPanel.MoveChildrenTo(panel1);
                    panel2.Children.Add(src.Element);
                    break;
                default:
                    throw new InvalidOperationException($"Unknown Tag [{hitPart}]");
                }
                dstPanel.SetGroups(panel1, panel2, (hitPart == "Left" || hitPart == "Right") ? Orientation.Horizontal : Orientation.Vertical);
            }
        }

        internal static void DoDropTab(DragData src, DragData dst)
        {
            src.Panel.Children.Remove(src.Element);
            var dstPanel = dst.Panel.State == AnyDockStates.Abandon ? dst.Panel.ParentPanel : dst.Panel; // in case collapsed
            int dstIdx = dstPanel.Children.IndexOf(dst.Element);
            dstPanel.Children.Insert(dstIdx, src.Element);
            dstPanel.MainTab.SelectedIndex = dstIdx;
        }


        public string SelfCheck(int level)
        {
            var prefix = new string('\t', level);
            string ret = "";
            ret += $"{prefix}AnyDock[{State}][{Children.Count} kids]\n";
            if (State == AnyDockStates.Tab)
            {
                foreach (var kid in Children)
                    ret += $"{prefix}- {(kid as Label).Content} \t [{(MainTab.SelectedContent == kid ? 'X': ' ')}]\n";
            }
            else
            {
                var label1 = Orientation == Orientation.Horizontal ? "Left " : "Up   ";
                var label2 = Orientation == Orientation.Horizontal ? "Right" : "Down ";
                ret += $"{prefix}- [{label1}]\n";
                ret += Group1.SelfCheck(level + 1);
                ret += $"{prefix}- [{label2}]\n";
                ret += Group2.SelfCheck(level + 1);
            }
            return ret;
        }

    }
}