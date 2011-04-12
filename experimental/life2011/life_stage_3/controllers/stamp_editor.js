// Copyright (c) 2011 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @file
 * The stamp editor object.  This object manages the UI connections for various
 * elements of the stamp editor panel, and makes the dynamic parts of the
 * panel's DOM.
 */


goog.provide('stamp');
goog.provide('stamp.Editor');

goog.require('goog.Disposable');
goog.require('goog.dom');
goog.require('goog.editor.Table');
goog.require('goog.events');
goog.require('goog.style');
goog.require('goog.ui.Zippy');

/**
 * Manages the data and interface for the stamp editor.
 * @param {!Element} noteContainer The element under which DOM nodes for
 *     the stamp editor should be added.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
stamp.Editor = function(editorContainer) {
  goog.events.EventTarget.call(this);
  this.parent_ = editorContainer;
};
goog.inherits(stamp.Editor, goog.events.EventTarget);

/**
 * A dictionary of all the DOM elements used by the stamp editor.
 * @type {Object<Element>}
 * @private
 */
stamp.Editor.prototype.domElements_ = {};

/**
 * The minimum number of rows and columns in the stamp editor table.
 */
stamp.Editor.prototype.MIN_ROW_COUNT = 3;
stamp.Editor.prototype.MIN_COLUMN_COUNT = 3;

/**
 * The ids used for elements in the DOM.  The stamp editor expects these
 * elements to exist.
 * @enum {string}
 */
stamp.Editor.DomIds = {
  ADD_COLUMN_BUTTON: 'add_column_button',
  ADD_ROW_BUTTON: 'add_row_button',
  CANCEL_BUTTON: 'cancel_button',
  OK_BUTTON: 'ok_button',
  REMOVE_COLUMN_BUTTON: 'remove_column_button',
  REMOVE_ROW_BUTTON: 'remove_row_button',
  STAMP_EDITOR_BUTTON: 'stamp_editor_button',
  STAMP_EDITOR_CONTAINER: 'stamp_editor_container',
  STAMP_EDITOR_PANEL: 'stamp_editor_panel'
};

/**
 * Panel events.  These are dispatched on the stamp editor container object
 * (usually a button that opens the panel), and indicate things like when the
 * panel is about to open, or when it has been closed using specific buttons.
 * @enum {string}
 */
stamp.Editor.Events = {
  PANEL_DID_CANCEL: 'panel_did_cancel',  // The Cancel button was clicked.
  PANEL_DID_SAVE: 'panel_did_save',  // The OK button was clicked.
  // Sent right before the editor panel will collapse.  Note that it is possible
  // for the event to be sent, but the panel will not actually close.  This can
  // happen, for example, if the panel open button gets a mouse-down, but does
  // not get a complete click event (no corresponding mouse-up).
  PANEL_WILL_CLOSE: 'panel_will_close',
  // Sent right before the editor panel will expand.  Note that it is possible
  // for the event to be sent, but the panel will not actually open.  This can
  // happen, for example, if the panel open button gets a mouse-down, but does
  // not get a complete click event (no corresponding mouse-up).
  PANEL_WILL_OPEN: 'panel_will_open'
};

/**
 * Attributes added to cells to cache certain parameters like aliveState.
 * @enum {string}
 * @private
 */
stamp.Editor.CellAttributes_ = {
  IS_ALIVE: 'isalive',  // Whether the cell is alive or dead.
  ENABLED_FONT_COLOR: 'black',
  DISABLED_FONT_COLOR: 'lightgray'
};

/**
 * Characters used to encode the stamp as a string.
 * @enum {string}
 * @private
 */
stamp.Editor.StringEncoding_ = {
  DEAD_CELL: '.',
  END_OF_ROW: '\n',
  LIVE_CELL: '*'
};

/**
 * Override of disposeInternal() to dispose of retained objects and unhook all
 * events.
 * @override
 */
stamp.Editor.prototype.disposeInternal = function() {
  for (elt in this.domElements_) {
    goog.events.removeAll(elt);
  }
  var tableCells =
      this.stampEditorTable_.element.getElementsByTagName(goog.dom.TagName.TD);
  for (var i = 0; i < tableCells.length; ++i) {
    goog.events.removeAll(tableCells[i]);
  }
  goog.events.removeAll(this.parent_);
  this.panel_ = null;
  this.parent_ = null;
  stamp.Editor.superClass_.disposeInternal.call(this);
}

/**
 * Fills out the TABLE structure for the stamp editor.  The stamp editor
 * can be resized, and handles clicks in its cells by toggling their state.
 * The resulting TABLE element will have the minumum number of rows and
 * columns, and be filled in with a default stamp that creates a glider.
 * @param {!Element<TABLE>} stampEditorTableElement The TABLE element that gets
 *     filled out with the editable cells.
 * @private
 */
stamp.Editor.prototype.makeStampEditorDom_ = function(stampEditorTableElement) {
  var domTable = goog.editor.Table.createDomTable(
      document,
      this.MIN_COLUMN_COUNT,
      this.MIN_ROW_COUNT,
      { 'borderWidth': 1, 'borderColor': 'white' });
  var tableStyle = {
    'borderCollpase': 'collapse',
    'borderSpacing': '0px',
    'borderStyle': 'solid'
  };

  goog.style.setStyle(domTable, tableStyle);
  var tableCells =
      domTable.getElementsByTagName(goog.dom.TagName.TD);
  this.initCells_(tableCells);
  goog.dom.appendChild(stampEditorTableElement, domTable);
  this.stampEditorTable_ = new goog.editor.Table(domTable);
}

/**
 * Initialize a list of cells to the "alive" state: sets the is-alive
 * attribute and the enclosed image element.  Fix up the various attributes
 * that goog.editor.Table sets on cells.
 * @param {!Array<Element>} cells The array of cells to initialize.
 * @private
 */
stamp.Editor.prototype.initCells_ = function(cells) {
  var cellStyle = {
    'padding': '0px'
  };
  for (var i = 0; i < cells.length; ++i) {
    var cell = cells[i];
    // The goog.editor.Table functions set the cell widths to 60px.
    cell.style.removeProperty('width');
    goog.style.setStyle(cell, cellStyle);
    this.setCellIsAlive(cell, false);
    /*
    cell.innerHTML = '<img src="img/dead_cell.png" ' +
                     'alt="Click to change state" />';
    cell.setAttribute(stamp.Editor.CellAttributes_.IS_ALIVE, false);
    */
    goog.events.listen(cell, goog.events.EventType.CLICK,
        this.toggleCellState, false, this);
  }
}

/**
 * Respond to a MOUSEDOWN event on the parent element.  This dispatches a
 * a PANEL_WILL* event based on the expanded state of the panel.
 * @param {!goog.events.Event} mousedownEvent The MOUSEDOWN event that
 *     triggered this handler.
 */
stamp.Editor.prototype.handleParentMousedown_ = function(mousedownEvent) {
  mousedownEvent.stopPropagation();
  var eventType;
  if (this.panel_.isExpanded()) {
    // About to close the panel.
    eventType = stamp.Editor.Events.PANEL_WILL_CLOSE;
  } else {
    eventType = stamp.Editor.Events.PANEL_WILL_OPEN;
  }
  this.dispatchEvent(new goog.events.Event(eventType));
}

/**
 * Return the current stamp expressed as a string.  The format loosely follows
 * the .LIF 1.05 "spec", where rows are delineated by a \n, a live cell is
 * represented by a '*' and a dead one by a '.'.
 */
stamp.Editor.prototype.getStampAsString = function() {
  var stampString = '';
  var rowCount = this.rowCount();
  for (var rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
    var row = this.stampEditorTable_.rows[rowIndex];
    for (var colIndex = 0; colIndex < row.columns.length; ++colIndex) {
      var cell = row.columns[colIndex];
      if (this.cellIsAlive(cell.element)) {
        stampString += stamp.Editor.StringEncoding_.LIVE_CELL;
      } else {
        stampString += stamp.Editor.StringEncoding_.DEAD_CELL;
      }
    }
    stampString += stamp.Editor.StringEncoding_.END_OF_ROW;
  }
  return stampString;
}

/**
 * Sets the current stamp baes on the stamp encoding in |stampString|.  The
 * format loosely follows the .LIF 1.05 "spec", where rows are delineated by a
 * \n, a live cell is represented by a '*' and a dead one by a '.'.
 * @param {!string} stampString The encoded stamp string.
 */
stamp.Editor.prototype.setStampFromString = function(stampStringIn) {
  if (stampStringIn.length == 0)
    return;  // Error?
  // The stamp string has to end with a END_OF_ROW character.
  var stampString = stampStringIn;
  if (stampString[stampString.length - 1] !=
      stamp.Editor.StringEncoding_.END_OF_ROW) {
    stampString += stamp.Editor.StringEncoding_.END_OF_ROW;
  }
  // Compute the width and height of the new stamp.
  var eorPos = stampString.indexOf(stamp.Editor.StringEncoding_.END_OF_ROW);
  var width = 0;
  var height = 0;
  if (eorPos == -1) {
    // The stamp string is a single row.
    width = stampString.length;
    height = 1;
  } else {
    width = eorPos;
    // Count up the number of rows in the encoded string.
    var rowCount = 0;
    do {
      ++rowCount;
      eorPos = stampString.indexOf(stamp.Editor.StringEncoding_.END_OF_ROW,
                                   eorPos + 1);
    } while (eorPos != -1);
    height = rowCount;
  }

  // Resize the editor.
  var currentWidth = this.columnCount();
  if (currentWidth < width) {
    for (var col = 0; col < width - currentWidth; ++col) {
      this.addColumn();
    }
  } else {
    var clampedWidth = width;
    if (width < this.MIN_COLUMN_COUNT) {
      clampedWidth = this.MIN_COLUMN_COUNT;
    }
    if (currentWidth > clampedWidth) {
      for (var col = 0; col < currentWidth - clampedWidth; ++col) {
        this.removeColumn();
      }
    }
  }
  var currentHeight = this.rowCount();
  if (currentHeight < height) {
    for (var row = 0; row < height - currentHeight; ++row) {
      this.addRow();
    }
  } else {
    var clampedHeight = height;
    if (height < this.MIN_ROW_COUNT) {
      clampedHeight = this.MIN_ROW_COUNT;
    }
    if (currentHeight > clampedHeight) {
      for (var row = 0; row < currentHeight - clampedHeight; ++row) {
        this.removeRow();
      }
    }
  }

  var rowIndex = 0;
  var columnIndex = 0;
  for (var i = 0; i < stampString.length; ++i) {
    var cell = this.cellAt(rowIndex, columnIndex);
    switch (stampString.charAt(i)) {
    case stamp.Editor.StringEncoding_.DEAD_CELL:
      this.setCellIsAlive(cell, false);
      ++columnIndex;
      break;
    case stamp.Editor.StringEncoding_.LIVE_CELL:
      this.setCellIsAlive(cell, true);
      ++columnIndex;
      break;
    case stamp.Editor.StringEncoding_.END_OF_ROW:
      ++rowIndex;
      columnIndex = 0;
      break;
    default:
      // Invalid character, set the cel to "dead".
      this.setCellIsAlive(cell, false);
      ++columnIndex;
      break;
    }
  }
}

/**
 * Creates the DOM structure for the stamp editor panel and adds it to the
 * document.  The panel is built into the mainPanel DOM element.  The expected
 * DOM elements are keys in the |stampEditorElements| object:
 *   mainPanel The main panel element.
 *   editorContainer: A container for a TABLE element that impelements the
 *       stamp editor.
 *   addColumnButton: An element that becomes the button used to add a column to
 *       the stamp editor.  An onclick listener is attached to this element.
 *   removeColumnButton:  An element that becomes the button used to remove a
 *       column from the stamp editor.  An onclick listener is attached to this
 *       element.
 *   addRowButton:  An element that becomes the button used to add a row to
 *       the stamp editor.  An onclick listener is attached to this element.
 *   removeRowButton: An element that becomes the button used to remove a
 *       row from the stamp editor.  An onclick listener is attached to this
 *       element.
 *   cancelButton: An element that becomes the button used to close the panel
 *       without changing the current stamp.  An onclick listener is attached
 *       to this element.  Dispatches the "panel did close" event.
 *   okButton:  An element that becomes the button used to close the panel
 *       and update the current stamp.  An onclick listener is attached
 *       to this element.  Dispatches the "panel did close" event.
 * @param {!Object<Element>} stampEditorElements A dictionary of DOM elements
 *     required by the stamp editor panel.
 * @return {!goog.ui.Zippy} The Zippy element representing the entire stamp
 *     panel with enclosed editor.
 */
stamp.Editor.prototype.makeStampEditorPanel = function(stampEditorElements) {
  if (!stampEditorElements) {
    return null;
  }
  for (elt in stampEditorElements) {
    this.domElements_[elt] = stampEditorElements[elt];
  }
  // Create DOM structure to represent the stamp editor panel.  This panel
  // contains all the UI elements of the stamp editor: the stamp editor
  // itself, add/remove column and row buttons, a legend and a title string.
  // The layout of the panel is described in the markup; this code wires up
  // the button and editor behaviours.
  this.domElements_.panelHeader = goog.dom.createDom('div',
      {'style': 'background-color:#EEE',
       'class': 'panel-container'}, 'Stamp Editor...');
  this.domElements_.panelContainer = stampEditorElements.mainPanel;
  goog.style.setPosition(this.domElements_.panelContainer, 0,
                         goog.style.getSize(this.parent_).height);
  this.domElements_.panelContainer.style.display = 'block';
  var newEditor = goog.dom.createDom('div', null,
      this.domElements_.panelHeader, this.domElements_.panelContainer);

  // Create the editable stamp representation within the editor panel.
  this.makeStampEditorDom_(stampEditorElements.editorContainer);

  goog.events.listen(this.parent_, goog.events.EventType.MOUSEDOWN,
                     this.handleParentMousedown_, false, this);

  // Wire up the add/remove column and row buttons.
  goog.events.listen(stampEditorElements.addColumnButton,
                     goog.events.EventType.CLICK,
                     this.addColumn, false, this);
  goog.events.listen(stampEditorElements.removeColumnButton,
                     goog.events.EventType.CLICK,
                     this.removeColumn, false, this);
  goog.events.listen(stampEditorElements.addRowButton,
                     goog.events.EventType.CLICK,
                     this.addRow, false, this);
  goog.events.listen(stampEditorElements.removeRowButton,
                     goog.events.EventType.CLICK,
                     this.removeRow, false, this);
  goog.events.listen(stampEditorElements.cancelButton,
                     goog.events.EventType.CLICK,
                     this.closePanelAndCancel, false, this);
  goog.events.listen(stampEditorElements.okButton,
                     goog.events.EventType.CLICK,
                     this.closePanelAndUpdate, false, this);

  // Add the panel's DOM structure to the document.
  goog.dom.appendChild(this.parent_, newEditor);
  this.panel_ = new goog.ui.Zippy(this.domElements_.panelHeader,
                                  this.domElements_.panelContainer);
  return this.panel_;
};

/**
 * Respond to a CLICK event in a table cell by toggling its state.
 * @param {!goog.events.Event} clickEvent The CLICK event that triggered this
 *     handler.
 */
stamp.Editor.prototype.toggleCellState = function(clickEvent) {
  clickEvent.stopPropagation();
  // The cell is the enclosing TD element.
  var cell = goog.dom.getAncestor(clickEvent.target, function(node) {
      return node.tagName && node.tagName.toUpperCase() == goog.dom.TagName.TD;
    });
  // TODO(dspringer): throw an error or assert if no enclosing TD element is
  // found.
  this.setCellIsAlive(cell, !this.cellIsAlive(cell));
}

/**
 * Return the TD element for the cell at location (|row|, |column|).
 * @param {number} rowIndex The row index.  This is 0-based.
 * @param {number} columnIndex The column index.  This is 0-based.
 * @return {?Element} The TD element representing the cell.  If no cell exists
 *     then return null.
 */
stamp.Editor.prototype.cellAt = function(rowIndex, columnIndex) {
  if (rowIndex < 0 || rowIndex >= this.stampEditorTable_.rows.length)
    return null;
  var row = this.stampEditorTable_.rows[rowIndex];
  if (columnIndex < 0 || columnIndex >= row.columns.length)
    return null;
  return row.columns[columnIndex].element;
}

/**
 * Respond to a CLICK event on the "add column" button.  Update the display
 * to reflect whether columns can be removed or not.
 * @param {!goog.events.Event} clickEvent The CLICK event that triggered this
 *     handler.
 */
stamp.Editor.prototype.addColumn = function(clickEvent) {
  clickEvent.stopPropagation();
  var newCells = this.stampEditorTable_.insertColumn();
  this.initCells_(newCells);
  if (this.columnCount() > this.MIN_COLUMN_COUNT) {
    goog.style.setStyle(this.domElements_.removeColumnButton,
        { 'color': stamp.Editor.CellAttributes_.ENABLED_FONT_COLOR });
  }
}

/**
 * Respond to a CLICK event on the "remove column" button.
 * @param {!goog.events.Event} clickEvent The CLICK event that triggered this
 *     handler.
 */
stamp.Editor.prototype.removeColumn = function(clickEvent) {
  clickEvent.stopPropagation();
  var columnCount = this.columnCount();
  if (columnCount <= this.MIN_COLUMN_COUNT) {
    return;
  }
  // Unhook all the listeners that have been attached to the cells in the
  // last column, then remove the column.
  for (var i = 0; i < this.stampEditorTable_.rows.length; ++i) {
    var row = this.stampEditorTable_.rows[i];
    var cell = row.columns[columnCount - 1];
    goog.events.removeAll(cell);
  }
  this.stampEditorTable_.removeColumn(columnCount - 1);
  // Update the UI if there are the minumum number of columns remaining.  Note
  // that the value of columnCount() could be different than when it was called
  // before removing the column.
  if (this.columnCount() == this.MIN_COLUMN_COUNT) {
    goog.style.setStyle(this.domElements_.removeColumnButton,
        { 'color': stamp.Editor.CellAttributes_.DISABLED_FONT_COLOR });
  }
}

/**
 * Return the number of columns in the stamp editor table.  This assumes that
 * there are no merged cells in row[0], and that the number of cells in all
 * rows is the same as the length of row[0].
 * @return {int} The number of columns.
 */
stamp.Editor.prototype.columnCount = function() {
  if (!this.stampEditorTable_)
    return 0;
  if (!this.stampEditorTable_.rows)
    return 0;
  if (!this.stampEditorTable_.rows[0].columns)
    return 0;
  return this.stampEditorTable_.rows[0].columns.length;
}

/**
 * Respond to a CLICK event on the "add row" button.
 * @param {!goog.events.Event} clickEvent The CLICK event that triggered this
 *     handler.
 */
stamp.Editor.prototype.addRow = function(clickEvent) {
  clickEvent.stopPropagation();
  var newTableRow = this.stampEditorTable_.insertRow();
  this.initCells_(goog.editor.Table.getChildCellElements(newTableRow));
  if (this.rowCount() > this.MIN_ROW_COUNT) {
    goog.style.setStyle(this.domElements_.removeRowButton,
        { 'color': stamp.Editor.CellAttributes_.ENABLED_FONT_COLOR });
  }
}

/**
 * Respond to a CLICK event on the "remove row" button.
 * @param {!goog.events.Event} clickEvent The CLICK event that triggered this
 *     handler.
 */
stamp.Editor.prototype.removeRow = function(clickEvent) {
  clickEvent.stopPropagation();
  var rowCount = this.rowCount();
  // If removing this row will result in less than the minumum number of
  // row, then update the UI to disable the remove row button and return
  // without removing the row.
  if (rowCount <= this.MIN_ROW_COUNT) {
    return;
  }
  // Unhook all the listeners that have been attached to the cells in the
  // last row, then remove the row.
  var lastRow = this.stampEditorTable_.rows[rowCount - 1];
  for (var i = 0; i < lastRow.columns.length; ++i) {
    var cell = lastRow.columns[i];
    goog.events.removeAll(cell);
  }
  this.stampEditorTable_.removeRow(rowCount - 1);
  // Update the UI if there are the minumum number of rows remaining.  Note
  // that the value of rowCount() could be different than when it was called
  // before removing the row.
  if (this.rowCount() == this.MIN_ROW_COUNT) {
    goog.style.setStyle(this.domElements_.removeRowButton,
        { 'color': stamp.Editor.CellAttributes_.DISABLED_FONT_COLOR });
  }
}

/**
 * Return the number of rows in the stamp editor table.  Assumes that there are
 * no merged cells in any columns.
 * @return {int} The number of rows.
 */
stamp.Editor.prototype.rowCount = function() {
  if (!this.stampEditorTable_)
    return 0;
  if (!this.stampEditorTable_.rows)
    return 0;
  return this.stampEditorTable_.rows.length;
}

/**
 * Respond to a CLICK event on the "cancel" button.
 * @param {!goog.events.Event} clickEvent The CLICK event that triggered this
 *     handler.
 */
stamp.Editor.prototype.closePanelAndCancel = function(clickEvent) {
  if (this.panel_.isExpanded())
    this.panel_.collapse();
  // Dispatch two events: PANEL_WILL_CLOSE and PANEL_DID_CANCEL.
  this.dispatchEvent(new goog.events.Event(stamp.Editor.PANEL_WILL_CLOSE));
  this.dispatchEvent(new goog.events.Event(stamp.Editor.PANEL_DID_CANCEL));
}

/**
 * Respond to a CLICK event on the "ok" button.
 * @param {!goog.events.Event} clickEvent The CLICK event that triggered this
 *     handler.
 */
stamp.Editor.prototype.closePanelAndUpdate = function(clickEvent) {
  if (this.panel_.isExpanded())
    this.panel_.collapse();
  // Dispatch two events: PANEL_WILL_CLOSE and PANEL_DID_SAVE.  The
  // PANEL_DID_SAVE event carries the stamp string aas part of its payload.
  this.dispatchEvent(new goog.events.Event(stamp.Editor.PANEL_WILL_CLOSE));
  var didSaveEvent = new goog.events.Event(stamp.Editor.PANEL_DID_SAVE);
  didSaveEvent.stampString = this.getStampAsString();
  this.dispatchEvent(didSaveEvent);
}

/**
 * Accessor for the is-alive state of a cell.
 * @param {!Element} cell The target cell.
 * @return {bool} The is-alive state of |cell|.
 */
stamp.Editor.prototype.cellIsAlive = function(cell) {
  isAlive = cell.getAttribute(stamp.Editor.CellAttributes_.IS_ALIVE);
  return isAlive != 'false';
}

/**
 * Change the is-alive state of a cell to |isAlive|.  The appearance of the cell
 * is also changed to match the new state.
 * @param {!Element} cell The target cell.
 * @param {bool} isAlive The new is-alive state of the cell.
 */
stamp.Editor.prototype.setCellIsAlive = function(cell, isAlive) {
  cell.setAttribute(stamp.Editor.CellAttributes_.IS_ALIVE, isAlive);
  var cellImg = isAlive ? 'img/live_cell.png' : 'img/dead_cell.png';
  cell.innerHTML = '<img src="' + cellImg + '" alt="Click to change state." />';
}
