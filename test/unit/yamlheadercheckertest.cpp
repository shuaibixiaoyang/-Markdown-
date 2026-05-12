// 文件说明：test\unit\yamlheadercheckertest.cpp
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "yamlheadercheckertest.h"
#include "yamlheaderchecker.h"

// 函数说明：实现 YamlHeaderCheckerTest::multipleOccurance 的核心逻辑，供当前模块调用。
void YamlHeaderCheckerTest::multipleOccurance()
{
    QString header = "---\n"
                     "a: *b*\n"
                     "---\n";
    QString body = "first line.\n" + header +
                   "second line.\n" + header;
    QString doc = header + body;
    YamlHeaderChecker checker(doc);
    QVERIFY(checker.hasHeader());
    QCOMPARE(checker.header(), header);
    QCOMPARE(checker.body(), body);
    QCOMPARE(checker.bodyOffset(), header.length());
}

// 函数说明：实现 YamlHeaderCheckerTest::HeaderOnlyDocument 的核心逻辑，供当前模块调用。
void YamlHeaderCheckerTest::HeaderOnlyDocument()
{
    QString header = "---\n"
                     "a: *b*\n"
                     "---\n";
    QString doc = header;
    YamlHeaderChecker checker(doc);
    QVERIFY(checker.hasHeader());
    QCOMPARE(checker.header(), header);
    QCOMPARE(checker.body(), QString(""));
    QCOMPARE(checker.bodyOffset(), header.length());
}

// 函数说明：实现 YamlHeaderCheckerTest::EmptyHeader 的核心逻辑，供当前模块调用。
void YamlHeaderCheckerTest::EmptyHeader()
{
    QString header = "---\n"
                     "---\n";
    QString body = "first line.\n"
                   "second line.\n";
    QString doc = header + body;
    YamlHeaderChecker checker(doc);
    QVERIFY(checker.hasHeader());
    QCOMPARE(checker.header(), header);
    QCOMPARE(checker.body(), body);
    QCOMPARE(checker.bodyOffset(), header.length());
}

// 函数说明：实现 YamlHeaderCheckerTest::partialHeader 的核心逻辑，供当前模块调用。
void YamlHeaderCheckerTest::partialHeader()
{
    QString header = "---\n"
                     "a: *b*\n"
                     "--\n";
    QString body = "first line.\n"
                   "second line.\n";
    QString doc = header + body;
    YamlHeaderChecker checker(doc);
    QVERIFY(!checker.hasHeader());
    QCOMPARE(checker.header(), QString(""));
    QCOMPARE(checker.body(), doc);
}

// 函数说明：实现 YamlHeaderCheckerTest::endMarkNotAtLineBegin 的核心逻辑，供当前模块调用。
void YamlHeaderCheckerTest::endMarkNotAtLineBegin()
{
    QString header = "---\n"
                     "a: *b*\n"
                     "some thing---\n";
    QString body = "first line.\n"
                   "second line.\n";
    QString doc = header + body;
    YamlHeaderChecker checker(doc);
    QVERIFY(!checker.hasHeader());
    QCOMPARE(checker.header(), QString(""));
    QCOMPARE(checker.body(), doc);
}

// 函数说明：实现 YamlHeaderCheckerTest::tailingSpaces 的核心逻辑，供当前模块调用。
void YamlHeaderCheckerTest::tailingSpaces()
{
    QString header = "---  \n"
                     "a: *b*\n"
                     "---  \t\n";
    QString body = "first line.\n"
                   "second line.\n";
    QString doc = header + body;
    YamlHeaderChecker checker(doc);
    QVERIFY(checker.hasHeader());
    QCOMPARE(checker.header(), header);
    QCOMPARE(checker.body(), body);
}

// 函数说明：实现 YamlHeaderCheckerTest::marksWithOtherCharacters 的核心逻辑，供当前模块调用。
void YamlHeaderCheckerTest::marksWithOtherCharacters()
{
    QString header = "---\n"
                     "a: *b*\n"
                     "---abc\n";
    QString body = "first line.\n"
                   "second line.\n";
    QString doc = header + body;
    YamlHeaderChecker checker(doc);
    QVERIFY(!checker.hasHeader());
    QCOMPARE(checker.header(), QString(""));
    QCOMPARE(checker.body(), doc);
}

// 函数说明：实现 YamlHeaderCheckerTest::dotMark 的核心逻辑，供当前模块调用。
void YamlHeaderCheckerTest::dotMark()
{
    QString header = "---\n"
                     "a: *b*\n"
                     "...\n";
    QString body = "first line.\n"
                   "second line.\n";
    QString doc = header + body;
    YamlHeaderChecker checker(doc);
    QVERIFY(checker.hasHeader());
    QCOMPARE(checker.header(), header);
    QCOMPARE(checker.body(), body);
    QCOMPARE(checker.bodyOffset(), header.length());
}

