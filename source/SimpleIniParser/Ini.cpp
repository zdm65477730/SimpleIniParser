// SimpleIniParser
// Copyright (C) 2019 Steven Mattera
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "Ini.hpp"
#include "IniOption.hpp"
#include "Trim.hpp"
#include <sstream>
#include <switch.h>

using namespace std;

namespace simpleIniParser {
    Ini::~Ini() {
        for (IniSection * section : sections) {
            if (section != nullptr) {
                delete section;
                section = nullptr;
            }
        }
        sections.clear();
    }

    string Ini::build() {
        string result;

        for (IniSection * section : sections) {
            result += section->build() + "\n";
        }

        return result;
    }

    IniSection * Ini::findSection(string name) {
        auto it = find_if(sections.begin(), sections.end(), [&name](const IniSection * obj) { return obj->value == name; });
        if (it == sections.end())
            return nullptr;

        return (*it);
    }

    bool Ini::writeToFile(string path) {
        FsFileSystem fs;
        if (R_FAILED(fsOpenSdCardFileSystem(&fs))) return false;

        fsFsDeleteFile(&fs, path.c_str());
        std::string content = build();
        fsFsCreateFile(&fs, path.c_str(), content.size(), 0);

        FsFile dest_handle;
	    if (R_FAILED(fsFsOpenFile(&fs, path.c_str(), FsOpenMode_Write, &dest_handle))) {
            fsFsClose(&fs);
            return false;
        }

        if (R_FAILED(fsFileWrite(&dest_handle, 0, content.c_str(), content.size(), FsWriteOption_Flush))) {
            fsFileClose(&dest_handle);
            fsFsClose(&fs);
            return false;
        }

        fsFileClose(&dest_handle);
        fsFsClose(&fs);
        return true;
    }

    Ini * Ini::parseFile(string path) {
        FsFileSystem fs;
        if (R_FAILED(fsOpenSdCardFileSystem(&fs))) return nullptr;

        FsFile src_handle;
        if (R_FAILED(fsFsOpenFile(&fs, path.c_str(), FsOpenMode_Read, &src_handle))) {
            fsFsClose(&fs);
            return nullptr;
        }

        s64 size = 0;
        if (R_FAILED(fsFileGetSize(&src_handle, &size))) {
            fsFileClose(&src_handle);
            fsFsClose(&fs);
            return nullptr;
        }

        u64 bytes_read = 0;
        std::string strBuf(size, '\0');
        if (R_FAILED(fsFileRead(&src_handle, 0, const_cast<char*>(strBuf.data()), size, FsReadOption_None, &bytes_read))) {
			fsFileClose(&src_handle);
            fsFsClose(&fs);
			return nullptr;
		}

        std::stringstream ss(strBuf);

        Ini * ini = new Ini();
        string line;
        while (std::getline(ss, line)) {
            trim(line);

            if (line.size() == 0)
                continue;

            IniSection * section = IniSection::parse(line);
            if (section != nullptr) {
                ini->sections.push_back(section);
            }
            else if (ini->sections.size() != 0 && ini->sections.back()->type == SECTION) {
                IniOption * option = IniOption::parse(line);

                if (option != nullptr)
                    ini->sections.back()->options.push_back(option);
            }
        }

        //file.close();
        fsFileClose(&src_handle);
        fsFsClose(&fs);

        return ini;
    }
}
