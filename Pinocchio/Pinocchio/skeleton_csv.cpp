#include "skeleton.h"

#include <fstream>
#include <istream>
#include <string>
#include <vector>

namespace Pinocchio {

    // See: https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c

    enum class CSVState {
        UnquotedField,
        QuotedField,
        QuotedQuote
    };

    const std::string WHITESPACE = " \n\r\t\f\v";

    std::string ltrim(const std::string & s) {
        size_t start = s.find_first_not_of(WHITESPACE);
        return (start == std::string::npos) ? "" : s.substr(start);
    }

    std::string rtrim(const std::string & s) {
        size_t end = s.find_last_not_of(WHITESPACE);
        return (end == std::string::npos) ? "" : s.substr(0, end + 1);
    }

    std::string trim(const std::string & s) {
        return rtrim(ltrim(s));
    }

    std::vector<std::string> readCSVRow(const std::string &row) {
        CSVState state = CSVState::UnquotedField;

        const char *vinit[] = {""};
        std::vector<std::string> fields(vinit, std::end(vinit));

        size_t i = 0; // index of the current field
        for (char c : row) {
            switch (state) {
                case CSVState::UnquotedField:
                    switch (c) {
                        case ',': // end of field
                                  fields.push_back(""); i++;
                                  break;
                        case '"': state = CSVState::QuotedField;
                                  break;
                        default:  fields[i].push_back(c);
                                  break; }
                    break;
                case CSVState::QuotedField:
                    switch (c) {
                        case '"': state = CSVState::QuotedQuote;
                                  break;
                        default:  fields[i].push_back(c);
                                  break; }
                    break;
                case CSVState::QuotedQuote:
                    switch (c) {
                        case ',': // , after closing quote
                                  fields.push_back(""); i++;
                                  state = CSVState::UnquotedField;
                                  break;
                        case '"': // "" -> "
                                  fields[i].push_back('"');
                                  state = CSVState::QuotedField;
                                  break;
                        default:  // end of quote
                                  state = CSVState::UnquotedField;
                                  break; }
                    break;
            }
        }
        return fields;
    }

    /// Read CSV file, Excel dialect. Accept "quoted fields ""with quotes"""
    std::vector<std::vector<std::string>> readCSV(std::istream &in) {
        std::vector<std::vector<std::string>> table;
        std::string row;
        while (!in.eof()) {
            std::getline(in, row);
            if (in.bad() || in.fail()) {
                break;
            }
            row = trim(row);
            if (!row.length()) continue;
            auto fields = readCSVRow(row);
            table.push_back(fields);
        }
        return table;
    }

    CsvFileSkeleton::CsvFileSkeleton(const std::string &filename) {

            std::ifstream csv_file(filename);
            std::vector<std::vector<std::string>> csv_data = readCSV(csv_file);

            std::vector<std::pair<std::string,std::string>> symmetries;
            std::vector<std::string> foot_bones;
            std::vector<std::string> fat_bones;

            int row_number = 0;
            std::vector<std::string> csv_header;
            for(auto const & row_data: csv_data) {
                if (!row_number) {
                    csv_header = row_data;
                    for (std::vector<std::string>::size_type i = 0; i != row_data.size(); i++) {
                        std::string & field_name = csv_header[i];
                        transform(field_name.begin(), field_name.end(), field_name.begin(), ::toupper); 
                    }
                } else {
                    std::string bone_name, parent_bone, symmetric_bone;
                    float x_coord, y_coord, z_coord;
                    bool is_foot, is_fat;

                    for (std::vector<std::string>::size_type i = 0; i != row_data.size(); i++) {
                        if (std::string("BONE") == csv_header[i])
                            bone_name = trim(row_data[i]);
                        if (std::string("X") == csv_header[i])
                            x_coord = std::stof(trim(row_data[i]));
                        if (std::string("Y") == csv_header[i])
                            y_coord = std::stof(trim(row_data[i]));
                        if (std::string("Z") == csv_header[i])
                            z_coord = std::stof(trim(row_data[i]));
                        if (std::string("PARENT") == csv_header[i])
                            parent_bone = trim(row_data[i]);
                        if (std::string("SYMMETRIC") == csv_header[i])
                            symmetric_bone = trim(row_data[i]);
                        if (std::string("FOOT") == csv_header[i])
                            is_foot = (trim(row_data[i]).length() != 0);
                        if (std::string("FAT") == csv_header[i])
                            is_fat = (trim(row_data[i]).length() != 0);
                    }
                    if (bone_name.length() > 0 && symmetric_bone.length() > 0) {
                        symmetries.push_back(make_pair(bone_name, symmetric_bone));
                    }
                    if (is_foot) {
                        foot_bones.push_back(bone_name);
                    }
                    if (is_fat) {
                        fat_bones.push_back(bone_name);
                    }

                    std::cout << "Bone: " << bone_name << " = " << x_coord << ", " << y_coord << ", " << z_coord << "; Parent = " << parent_bone << std::endl;
                    makeJoint(bone_name,  Vector3(x_coord, y_coord, z_coord), parent_bone);

                }
                row_number++;
            }

            for (auto const & symmetry: symmetries) {
                std::cout << "Symmetry: " << symmetry.first << " <-> " << symmetry.second << std::endl;
                makeSymmetric(symmetry.first, symmetry.second);
            }

            initCompressed();

            for (auto const & bone: foot_bones) {
                std::cout << "Foot Bone: " << bone << std::endl;
                setFoot(bone);
            }

            for (auto const & bone: fat_bones) {
                std::cout << "Fat Bone: " << bone << std::endl;
                setFat(bone);
            }

    }

} // namespace Pinocchio
