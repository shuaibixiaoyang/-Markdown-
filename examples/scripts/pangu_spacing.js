// @name 中英文间距修复
// @description 自动在中文和英文/数字之间添加空格（盘古之白）
// @author CuteMarkEd
// @version 1.0
// @shortcut Ctrl+Shift+P

var text = editor.selectedText || editor.text;

// 中文后面紧跟英文或数字
var result = text.replace(/([\u4e00-\u9fff])([A-Za-z0-9])/g, '$1 $2');
// 英文或数字后面紧跟中文
result = result.replace(/([A-Za-z0-9])([\u4e00-\u9fff])/g, '$1 $2');

if (editor.selectedText) {
    editor.replaceSelection(result);
} else {
    editor.text = result;
}

var count = text.length !== result.length ? (result.length - text.length) : 0;
app.showMessage('已添加 ' + count + ' 个空格', 3000);
