####################################################################
#Dtool_funcToMethod(func, class)        
#del func
#####################################################################

"""
Mat3-extensions module: contains methods to extend functionality
of the LMatrix3f class.
"""


def pPrintValues(self):
        """
        Pretty print
        """
        return "\n%s\n%s\n%s" % (
            self.getRow(0).pPrintValues(), self.getRow(1).pPrintValues(), self.getRow(2).pPrintValues())
Dtool_funcToMethod(pPrintValues, Mat3)        
del pPrintValues
#####################################################################
