// 文件说明：app-static\yamlheaderchecker.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Copyright 2015 Aetf <7437103@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef YAMLHEADERCHECKER_H
#define YAMLHEADERCHECKER_H

#include <QString>

class YamlHeaderChecker
{
public:
    YamlHeaderChecker(const QString &text);

    bool hasHeader() const;
    QString header() const;
    QString body() const;

    int bodyOffset() const;

private:
    QString m_header;
    QString m_body;
};

#endif // YAMLHEADERCHECKER_H

