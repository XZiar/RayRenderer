
class COLOR:
    black	= "\033[90m"
    red		= "\033[91m"
    green	= "\033[92m"
    yellow	= "\033[93m"
    blue	= "\033[94m"
    magenta	= "\033[95m"
    cyan	= "\033[96m"
    white	= "\033[97m"
    clear	= "\033[39m"
    @staticmethod
    def Black(val):
        return COLOR.black  + str(val) + COLOR.clear
    @staticmethod
    def Red(val):
        return COLOR.red    + str(val) + COLOR.clear
    @staticmethod
    def Green(val):
        return COLOR.green  + str(val) + COLOR.clear
    @staticmethod
    def Yellow(val):
        return COLOR.yellow + str(val) + COLOR.clear
    @staticmethod
    def Blue(val):
        return COLOR.blue   + str(val) + COLOR.clear
    @staticmethod
    def Magenta(val):
        return COLOR.magenta+ str(val) + COLOR.clear
    @staticmethod
    def Cyan(val):
        return COLOR.cyan   + str(val) + COLOR.clear
    @staticmethod
    def White(val):
        return COLOR.white  + str(val) + COLOR.clear

def writeItems(file, name:str, val:list, state = ":"):
    file.write("{}\t{}= {}\n".format(name, state, " ".join(val)))
def writeItem(file, name:str, val:str, state = ":"):
    file.write("{}\t{}= {}\n".format(name, state, val))
    