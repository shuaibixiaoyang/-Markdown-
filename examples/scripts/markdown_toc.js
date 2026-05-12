// @name Markdown 目录生成器
// @description 扫描文档中的标题，在光标位置插入目录
// @author CuteMarkEd
// @version 1.0
// @shortcut Ctrl+Shift+G

var text = editor.text;
var lines = text.split('\n');
var toc = [];

for (var i = 0; i < lines.length; i++) {
    var line = lines[i];
    // 匹配 # 开头的标题
    var match = line.match(/^(#{1,6})\s+(.+)/);
    if (match) {
        var level = match[1].length;
        var title = match[2].trim();
        // 生成锚点（去掉特殊字符，空格换成连字符）
        var anchor = title.toLowerCase()
            .replace(/[^\w\u4e00-\u9fff\s-]/g, '')
            .replace(/\s+/g, '-');
        var indent = util.repeat('  ', level - 1);
        toc.push(indent + '- [' + title + '](#' + anchor + ')');
    }
}

if (toc.length > 0) {
    var result = '## 目录\n\n' + toc.join('\n') + '\n\n';
    editor.insertText(result);
    app.showMessage('已插入目录（' + toc.length + ' 个标题）', 3000);
} else {
    app.showMessage('文档中没有找到标题', 3000);
}
