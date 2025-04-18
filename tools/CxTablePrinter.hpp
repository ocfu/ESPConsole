#ifndef CxTablePrinter_hpp
#define CxTablePrinter_hpp

#include <vector>

class CxTablePrinter {
private:
   std::vector<uint8_t> columnWidths;
   uint8_t currentColumn = 0;
   Stream& output; // Reference to a Stream object
   
   // Helper to truncate strings
   String truncateString(const String& str, uint8_t width) {
      if (str.length() > width) {
         return str.substring(0, width - 3) + "...";
      }
      return str;
   }

   
public:
   // Constructor accepting a Stream reference
   CxTablePrinter(Stream& stream) : output(stream) {}
   
   void printHeader(const char* title, uint8_t width) {
      columnWidths.push_back(width);
      if (currentColumn > 0) {
         output.print(" | ");
      }
      output.printf("%-*s", width, title);
      currentColumn++;
   }
   
   void printHeader(const std::vector<String>& titles, const std::vector<uint8_t>& widths) {
      columnWidths = widths; // Store the widths
      for (size_t i = 0; i < titles.size(); i++) {
         if (i > 0) {
            output.print(" | ");
         }
         String truncated = truncateString(titles[i], widths[i]);
         output.printf("%-*s", widths[i], truncated.c_str());
      }
      printHeaderEnd();
   }

   void printHeaderEnd() {
      output.println();
      for (size_t i = 0; i < columnWidths.size(); i++) {
         if (i > 0) {
            output.print("-+-");
         }
         for (uint8_t j = 0; j < columnWidths[i]; j++) {
            output.print('-');
         }
      }
      output.println();
   }
   
   void printRow(const char* value) {
      if (currentColumn >= columnWidths.size()) {
         currentColumn = 0; // Reset to the first column
      }
      if (currentColumn > 0) {
         output.print(" | ");
      }
      output.printf("%-*s", columnWidths[currentColumn], value);
      currentColumn++;
   }
   
   void printRow(const std::vector<String>& values) {
      for (size_t i = 0; i < values.size(); i++) {
         if (i > 0) {
            output.print(" | ");
         }
         String truncated = truncateString(values[i], columnWidths[i]);
         output.printf("%-*s", columnWidths[i], truncated.c_str());
      }
      printRowEnd();
   }

   void printRowEnd() {
      output.println();
      currentColumn = 0; // Reset for the next row
   }
};
#endif /* CxTablePrinter_hpp */
