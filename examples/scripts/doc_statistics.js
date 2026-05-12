// @name 文档统计报告
// @description 生成详细的文档统计信息（字数、段落、标题等）
// @author CuteMarkEd
// @version 1.0
// @shortcut Ctrl+Shift+I

var text = editor.text;
var lines = text.split('\n');

// 基础统计
var charCount = text.length;
var charNoSpace = text.replace(/\s/g, '').length;
var lineCount = lines.length;

// 中文字数
var chineseChars = text.match(/[\u4e00-\u9fff]/g);
var chineseCount = chineseChars ? chineseChars.length : 0;

// 英文单词数
var englishWords = text.match(/[a-zA-Z]+/g);
var englishWordCount = englishWords ? englishWords.length : 0;

// 段落数（空行分隔）
var paragraphCount = 0;
var inParagraph = false;
for (var i = 0; i < lines.length; i++) {
    if (lines[i].trim().length > 0) {
        if (!inParagraph) {
            paragraphCount++;
            inParagraph = true;
        }
    } else {
        inParagraph = false;
    }
}

// 标题统计
var h1 = 0, h2 = 0, h3 = 0, h4 = 0, h5 = 0, h6 = 0;
for (var i = 0; i < lines.length; i++) {
    var m = lines[i].match(/^(#{1,6})\s/);
    if (m) {
        switch (m[1].length) {
            case 1: h1++; break;
            case 2: h2++; break;
            case 3: h3++; break;
            case 4: h4++; break;
            case 5: h5++; break;
            case 6: h6++; break;
        }
    }
}
var headingTotal = h1 + h2 + h3 + h4 + h5 + h6;

// 链接和图片
var links = text.match(/\[([^\]]*)\]\([^)]+\)/g);
var linkCount = links ? links.length : 0;
var images = text.match(/!\[([^\]]*)\]\([^)]+\)/g);
var imageCount = images ? images.length : 0;

// 代码块
var codeBlocks = text.match(/```[\s\S]*?```/g);
var codeBlockCount = codeBlocks ? codeBlocks.length : 0;

// 估算阅读时间（中文 300字/分钟，英文 200词/分钟）
var readMinutes = Math.ceil(chineseCount / 300 + englishWordCount / 200);
if (readMinutes < 1) readMinutes = 1;

// 生成报告
var report = '📊 文档统计报告\n'
    + '════════════════════════\n'
    + '总字符数：　' + charCount + '\n'
    + '不含空格：　' + charNoSpace + '\n'
    + '中文字数：　' + chineseCount + '\n'
    + '英文单词：　' + englishWordCount + '\n'
    + '总行数：　　' + lineCount + '\n'
    + '段落数：　　' + paragraphCount + '\n'
    + '────────────────────────\n'
    + '标题数：　　' + headingTotal;

if (headingTotal > 0) {
    report += ' (';
    var parts = [];
    if (h1) parts.push('H1:' + h1);
    if (h2) parts.push('H2:' + h2);
    if (h3) parts.push('H3:' + h3);
    if (h4) parts.push('H4:' + h4);
    if (h5) parts.push('H5:' + h5);
    if (h6) parts.push('H6:' + h6);
    report += parts.join(', ') + ')';
}
report += '\n';

report += '链接数：　　' + linkCount + '\n'
    + '图片数：　　' + imageCount + '\n'
    + '代码块数：　' + codeBlockCount + '\n'
    + '────────────────────────\n'
    + '预估阅读时间：约 ' + readMinutes + ' 分钟\n';

app.showInfo('文档统计', report);
