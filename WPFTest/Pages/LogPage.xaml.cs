using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
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
using Xceed.Wpf.Toolkit;
using XZiar.Util;
using XZiar.WPFControl;

namespace WPFTest
{
    internal class LogColorConverter : IValueConverter
    {
        private static readonly SolidColorBrush BrushError   = new SolidColorBrush(Colors.Red);
        private static readonly SolidColorBrush BrushWarning = new SolidColorBrush(Colors.Yellow);
        private static readonly SolidColorBrush BrushSuccess = new SolidColorBrush(Colors.LawnGreen);
        private static readonly SolidColorBrush BrushVerbose = new SolidColorBrush(Colors.Pink);
        private static readonly SolidColorBrush BrushDebug   = new SolidColorBrush(Colors.Cyan);
        private static readonly SolidColorBrush BrushInfo    = new SolidColorBrush(Colors.White);

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            switch ((common.LogLevel)value)
            {
            case common.LogLevel.Error:     return BrushError;
            case common.LogLevel.Warning:   return BrushWarning;
            case common.LogLevel.Success:   return BrushSuccess;
            case common.LogLevel.Verbose:   return BrushVerbose;
            case common.LogLevel.Debug:     return BrushDebug;
            case common.LogLevel.Info:
            default:                        return BrushInfo;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// LogPage.xaml 的交互逻辑
    /// </summary>
    public partial class LogPage : ContentControl
    {
        public class LogItem
        {
            public common.LogLevel Level { get; private set; }
            public string Source { get; private set; }
            public string Text { get; private set; }
            public LogItem(common.LogLevel level, string source, string text)
            {
                Level = level; Source = source; Text = text;
            }
        }

        private bool SelfUpdate = false;
        private readonly ObservableCollection<LogItem> LogItems = new ObservableCollection<LogItem>();
        private readonly ObservableSelection<string> FilterFrom = new ObservableSelection<string>();
        private readonly BitArray FilterLevels = new BitArray(sizeof(common.LogLevel) * 256);

        private readonly CollectionView LogsView;
        private ScrollViewer Scroller;

        public LogPage()
        {
            InitializeComponent();
            Loaded += LogPageLoaded;

            LevelFilter.ItemsSource = Enum.GetValues(typeof(common.LogLevel)).Cast<common.LogLevel>().ToList();
            SelfUpdate = true;
            LevelFilter.SelectAll();
            foreach (int lv in LevelFilter.SelectedItems.Cast<byte>())
                FilterLevels.Set(lv, true);
            SelfUpdate = false;
            SourceFilter.ItemsSource = FilterFrom;
            common.Logger.OnLog += OnNewLog;

            logListView.ItemsSource = LogItems;
            LogsView = (CollectionView)CollectionViewSource.GetDefaultView(logListView.ItemsSource);
            LogsView.Filter = LogFilter;
        }

        private void LogPageLoaded(object sender, RoutedEventArgs e)
        {
            Scroller = WPFTreeHelper.GetChildrenOfType<ScrollViewer>(logListView).First();
            Loaded -= LogPageLoaded;
        }

        private bool LogFilter(object obj)
        {
            var item = (LogItem)obj;
            return FilterLevels.Get((byte)item.Level) && FilterFrom.Contains(item.Source, true);
        }

        private void OnChangeSource(object sender, Xceed.Wpf.Toolkit.Primitives.ItemSelectionChangedEventArgs e)
        {
            if (SelfUpdate) return;
            FilterFrom.RefreshSelection(SourceFilter.SelectedItems.Cast<string>());
            LogsView.Refresh();
        }

        private void OnChangeLevel(object sender, Xceed.Wpf.Toolkit.Primitives.ItemSelectionChangedEventArgs e)
        {
            if (SelfUpdate) return;
            FilterLevels.SetAll(false);
            foreach (int lv in LevelFilter.SelectedItems.Cast<byte>())
                FilterLevels.Set(lv, true);
            LogsView.Refresh();
        }

        private void ScrollCheck()
        {
            if (Scroller != null && Scroller.ScrollableHeight - Scroller.VerticalOffset < 120)
                Scroller.ScrollToEnd();
        }

        private void InnerAppendLog(common.LogLevel level, string from, string content)
        {
            var item = new LogItem(level, from, content);
            LogItems.Add(item);
            if (FilterFrom.Add(from, true))
            {
                SelfUpdate = true;
                SourceFilter.SelectedItems.Add(from);
                SelfUpdate = false;
            }
            else if (!LogFilter(item))
                return;
            ScrollCheck();
        }

        public void OnNewLog(common.LogLevel level, string from, string content)
        {
            Dispatcher.InvokeAsync(() => InnerAppendLog(level, from, content.Trim()), 
                System.Windows.Threading.DispatcherPriority.Normal);
        }
    }
}
