// 文件说明：app-static\yamlheaderchecker.cpp
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
#include <QRegularExpression>
#include "yamlheaderchecker.h"

// 函数说明：构造 YamlHeaderChecker 对象，初始化本模块需要的状态、界面和资源。
YamlHeaderChecker::YamlHeaderChecker(const QString &text)
{
    QRegularExpression rx(R"(^---\s*\n(.*?\n)?(---|\.\.\.)\s*(\n|$))",
                          QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = rx.match(text);
    if (match.hasMatch()) {
        m_header = match.captured(0);
        m_body = text.mid(m_header.length());
    } else {
        m_body = text;
    }
}

// 函数说明：检查 YamlHeaderChecker 是否具备对应的数据或能力。
bool YamlHeaderChecker::hasHeader() const
{
    return !m_header.isEmpty();
}

// 函数说明：实现 YamlHeaderChecker::header 的核心逻辑，供当前模块调用。
QString YamlHeaderChecker::header() const
{
    return m_header;
}

// 函数说明：实现 YamlHeaderChecker::body 的核心逻辑，供当前模块调用。
QString YamlHeaderChecker::body() const
{
    return m_body;
}

// 函数说明：实现 YamlHeaderChecker::bodyOffset 的核心逻辑，供当前模块调用。
int YamlHeaderChecker::bodyOffset() const
{
    return m_header.length();
}

