#ifndef CxTablePrinter_hpp
#define CxTablePrinter_hpp

#include <vector>

class CxTablePrinter {
private:
   std::vector<uint8_t> _vColumnWidths;
   uint8_t _nCurrentColumn = 0;
   Stream& _output; // Reference to a Stream object
   const char* _szName;
   uint16_t _nLines;
   
   // Helper to truncate strings
   String truncateString(const String& str, uint8_t width) {
      if (str.length() > width) {
         return str.substring(0, width - 3) + "...";
      }
      return str;
   }

   
public:
   // Constructor accepting a Stream reference
   explicit CxTablePrinter(Stream& stream, const char* name = nullptr) : _output(stream), _szName(name), _nLines(0) {}
   
   void printHeader(const std::vector<String>& titles, const std::vector<uint8_t>& widths) {
      _vColumnWidths = widths; // Store the widths
      _output.print(ESC_ATTR_BOLD);
      printLine(false);
#ifndef MINIMAL_COMMAND_SET
      // print centered name of the table, if given
      if (_szName) {
         uint16_t nLen = 0;
         for (auto& w : widths) {
            nLen += w;
         }
         nLen /= 2;
         nLen -= strlen(_szName)/2;
         while (nLen--) _output.print(" ");
         _output.println(_szName);
         printLine(false);
      }
#endif
      
      for (size_t i = 0; i < titles.size(); i++) {
         if (i > 0) {
            _output.print(" | ");
         }
         String truncated = truncateString(titles[i], widths[i]);
         _output.printf("%-*s", widths[i], truncated.c_str());
      }
      _output.println();
      printLine();
      _output.print(ESC_ATTR_RESET);
   }

   void printLine(bool bDelimiter = true) {
#ifndef MINIMAL_COMMAND_SET
      for (size_t i = 0; i < _vColumnWidths.size(); i++) {
         if (i > 0) {
            _output.print(bDelimiter?"-+-":"---");
         }
         for (uint8_t j = 0; j < _vColumnWidths[i]; j++) {
            _output.print('-');
         }
      }
      _output.println();
#endif
   }
   
   void printRow(const std::vector<String>& values) {
      for (size_t i = 0; i < values.size(); i++) {
         if (i > 0) {
            _output.print(ESC_ATTR_BOLD " | " ESC_ATTR_RESET);
         }
         String truncated = truncateString(values[i], _vColumnWidths[i]);
         _output.printf("%-*s", _vColumnWidths[i], truncated.c_str());
      }
      _output.println();
      _nCurrentColumn = 0; // Reset for the next row
      _nLines++;
   }
   
   void printFooter() {
#ifndef MINIMAL_COMMAND_SET
      _output.print(ESC_ATTR_BOLD);
      printLine(false);
      _output.print(_nLines);
      _output.println(F(" rows"));
#endif
   }
};
#endif /* CxTablePrinter_hpp */
