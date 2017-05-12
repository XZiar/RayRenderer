using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using RayRender;

namespace WPFTest
{
    public static class Main
    {
        public enum OPObject { Camera, Light, Object };
        public static BasicTest test;
        private static ushort curObj_ = 0;
        public static ushort curObj
        {
            get { return curObj_; }
            set
            {
                if (value == ushort.MaxValue)
                    curObj_ = (ushort)(test.objectCount - 1);
                else if(value >= test.objectCount)
                    curObj_ = 0;
                else 
                    curObj_ = value;
            }
        }
        public static bool isAnimate = false;
        public static bool mode
        {
            get { return test.mode; }
            set { test.mode = value; }
        }

        static Main()
        {
            test = new BasicTest();
        }
        public static void Move(float x, float y, float z, OPObject obj)
        {
            switch(obj)
            {
            case OPObject.Object:
                test.Moveobj(curObj, x, y, z);
                break;
            case OPObject.Camera:
                test.cam.Move(x, y, z);
                break;
            case OPObject.Light:
                break;
            }
        }
        public static void Rotate(float x, float y, float z, OPObject obj)
        {
            switch (obj)
            {
            case OPObject.Object:
                test.Rotateobj(curObj, x, y, z);
                break;
            case OPObject.Camera:
                test.cam.Pitch(x);
                test.cam.Yaw(y);
                test.cam.Roll(z);
                break;
            case OPObject.Light:
                break;
            }
        }
    }
}
