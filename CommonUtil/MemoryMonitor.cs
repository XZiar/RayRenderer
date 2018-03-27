using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Text;
using System.Threading;

namespace XZiar.Util
{
    public class MemoryMonitor : BaseViewModel
    {
        private static void UpdateInfo(object obj)
        {
            var monitor = (MemoryMonitor)obj;
            if (monitor.CombineNotify)
                monitor.OnPropertyChanged("Current");
            else
            {
                monitor.OnPropertyChanged("PrivateSize");
                monitor.OnPropertyChanged("ManagedSize");
                monitor.OnPropertyChanged("WorkingSet");
            }
        }
        private readonly Process CurProc = Process.GetCurrentProcess();
        private readonly Timer UpdateTimer;
        private bool CombineNotify = false;

        public ulong ManagedSize => (ulong)GC.GetTotalMemory(false);
        public ulong PrivateSize => (ulong)CurProc.PrivateMemorySize64;
        public ulong WorkingSet => (ulong)CurProc.WorkingSet64;
        public MemoryMonitor Current => this;

        public MemoryMonitor()
        {
            UpdateTimer = new Timer(UpdateInfo, this, Timeout.Infinite, 1000);
        }

        public void UpdateInterval(uint interval, bool combine)
        {
            if (interval > 0)
                UpdateTimer.Change(0, interval);
            else
                UpdateTimer.Change(Timeout.Infinite, 1000);
            CombineNotify = combine;
        }
    }
}
