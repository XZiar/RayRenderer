using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinFormTest
{
    static class Program
    {
        static Program()
        {
            AppDomain.CurrentDomain.AssemblyLoad += loadDLL;
            AppDomain.CurrentDomain.AssemblyResolve += resolveDLL;
            Console.WriteLine(Environment.Is64BitProcess ? "current in x64" : "current in x86");
        }

        private static Assembly resolveDLL(object sender, ResolveEventArgs args)
        {
            var tgEnv = Environment.Is64BitProcess ? ".x64.dll" : ".x86.dll";
            Console.WriteLine($"resolve Assembly {args.Name}");
            var dllname = args.Name.Substring(0, args.Name.IndexOf(',')) + tgEnv;
            return Assembly.LoadFrom(dllname);
        }

        private static void loadDLL(object sender, AssemblyLoadEventArgs args)
        {
            Console.WriteLine($"loaded Assembly {args.LoadedAssembly}");
        }
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
