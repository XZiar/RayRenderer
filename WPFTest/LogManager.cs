using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Media;
using Xceed.Wpf.Toolkit;
using XZiar.Util;

namespace WPFTest
{
    class LogManager
    {
        [StructLayout(LayoutKind.Auto)]
        public struct LogItem
        {
            public readonly common.LogLevel Level;
            public readonly string Source;
            public readonly Run R;
            public LogItem(common.LogLevel level, string source, Run run)
            {
                Level = level; Source = source; R = run;
            }
        }

        private static readonly SortedDictionary<common.LogLevel, SolidColorBrush> BrashMap = new SortedDictionary<common.LogLevel, SolidColorBrush>
        {
            { common.LogLevel.Error,   new SolidColorBrush(Colors.Red)     },
            { common.LogLevel.Warning, new SolidColorBrush(Colors.Yellow)  },
            { common.LogLevel.Success, new SolidColorBrush(Colors.LawnGreen)   },
            { common.LogLevel.Info,    new SolidColorBrush(Colors.White)   },
            { common.LogLevel.Verbose, new SolidColorBrush(Colors.Pink)    },
            { common.LogLevel.Debug,   new SolidColorBrush(Colors.Cyan)    }
        };

        private bool SelfUpdate = false;
        private readonly ObservableCollection<LogItem> LogItems = new ObservableCollection<LogItem>();
        private readonly ObservableSelection<string> FilterFrom = new ObservableSelection<string>();
        //private readonly HashSet<string> FilterSources = new HashSet<string>();
        private readonly BitArray FilterLevels = new BitArray(sizeof(common.LogLevel) * 256);
        //private readonly ObservableCollection<string> SourcesHolder = new ObservableCollection<string>();

        private readonly CheckComboBox LevelFilter, SourceFilter;
        private readonly Paragraph Para;
        private readonly ScrollViewer Scroller;

        public LogManager(CheckComboBox levelFilter, CheckComboBox sourceFilter, Paragraph para, ScrollViewer scrollViewer)
        {
            LevelFilter = levelFilter;
            LevelFilter.ItemsSource = Enum.GetValues(typeof(common.LogLevel)).Cast<common.LogLevel>().ToList();
            LevelFilter.ItemSelectionChanged += OnChangeLevel;
            SelfUpdate = true;
            LevelFilter.SelectAll();
            foreach (int lv in LevelFilter.SelectedItems.Cast<byte>())
                FilterLevels.Set(lv, true);
            SelfUpdate = false;
            SourceFilter = sourceFilter;
            SourceFilter.ItemsSource = FilterFrom;// SourcesHolder;
            SourceFilter.ItemSelectionChanged += OnChangeSource;

            Para = para;
            Scroller = scrollViewer;
            common.Logger.OnLog += OnNewLog;
        }

        private void OnChangeSource(object sender, Xceed.Wpf.Toolkit.Primitives.ItemSelectionChangedEventArgs e)
        {
            if (SelfUpdate) return;
            FilterFrom.RefreshSelection(SourceFilter.SelectedItems.Cast<string>());
            InnerRefresh();
        }

        private void OnChangeLevel(object sender, Xceed.Wpf.Toolkit.Primitives.ItemSelectionChangedEventArgs e)
        {
            if (SelfUpdate) return;
            FilterLevels.SetAll(false);
            foreach (int lv in LevelFilter.SelectedItems.Cast<byte>())
                FilterLevels.Set(lv, true);
            InnerRefresh();
        }

        private void ScrollCheck()
        {
            if (Scroller.ScrollableHeight - Scroller.VerticalOffset < 120)
                Scroller.ScrollToEnd();
        }

        private bool CheckFilter(common.LogLevel level, string from)
        {
            return FilterLevels.Get((byte)level) && FilterFrom.Contains(from, true);
        }

        private void InnerAppendLog(common.LogLevel level, string from, string content)
        {
            Run r = new Run($"[{from}]{content}")
            {
                Foreground = BrashMap[level]
            };
            LogItems.Add(new LogItem(level, from, r));
            if (FilterFrom.Add(from, true))
            {
                SelfUpdate = true;
                SourceFilter.SelectedItems.Add(from);
                SelfUpdate = false;
            }
            else if (!CheckFilter(level, from))
                return;
            Para.Inlines.Add(r);
            ScrollCheck();
        }
        private void InnerRefresh()
        {
            Para.Inlines.Clear();
            var mtched = LogItems.Where(x => CheckFilter(x.Level, x.Source)).Select(x => x.R);
            Para.Inlines.AddRange(mtched);
        }
        public void OnNewLog(common.LogLevel level, string from, string content)
        {
            Scroller.Dispatcher.InvokeAsync(() => InnerAppendLog(level, from, content),
                    System.Windows.Threading.DispatcherPriority.Normal);
        }
    }
}
