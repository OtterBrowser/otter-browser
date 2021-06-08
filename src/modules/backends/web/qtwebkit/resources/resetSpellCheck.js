const value = this.value;
const selectionStart = this.selectionStart;
const selectionEnd = this.selectionEnd;
const selectionDirection = this.selectionDirection;

this.value = '';
this.value = value;
this.setSelectionRange(selectionStart, selectionEnd, selectionDirection);
