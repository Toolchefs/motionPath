#				 Toolchefs ltd - Software Disclaimer
#
# Copyright 2014 Toolchefs Limited
#
# The software, information, code, data and other materials (Software)
# contained in, or related to, these files is the confidential and proprietary
# information of Toolchefs ltd.
# The software is protected by copyright. The Software must not be disclosed,
# distributed or provided to any third party without the prior written
# authorisation of Toolchefs ltd.

import os
import traceback

try:
    from PySide import QtGui, QtCore
    import PySide.QtGui as QtWidgets
except ImportError:
    from PySide2 import QtGui, QtCore, QtWidgets

from maya.app.general.mayaMixin import MayaQWidgetDockableMixin
from maya import OpenMayaUI
from maya import OpenMaya
from maya import cmds

WCN = "ToolchefsMotionPathWorkspaceControl"

class StaticLabels:
    FRAMES_BEFORE   = "ToolChefs_MP_framesBefore"
    FRAMES_AFTER    = "ToolChefs_MP_framesAfter"
    PATH_COLOR      = "ToolChefs_MP_pathColor"
    WPATH_COLOR     = "ToolChefs_MP_weightedPathColor"
    WTANGENT_COLOR  = "ToolChefs_MP_weightedPathTangentColor"
    BPATH_COLOR     = "ToolChefs_MP_bufferPathColor"
    CFRAME_COLOR    = "ToolChefs_MP_cFrameColor"
    TANGENT_COLOR   = "ToolChefs_MP_tangentColor"
    BTANGENT_COLOR  = "ToolChefs_MP_bTangentColor"
    PATH_SIZE       = "ToolChefs_MP_pathSize"
    FRAME_SIZE      = "ToolChefs_MP_frameSize"
    SHOW_PATH       = "ToolChefs_MP_showPath"
    SHOW_TANGENTS   = "ToolChefs_MP_showTangents"
    SHOW_KEYFRAMES  = "ToolChefs_MP_showKeyFrames"
    TIME_DELTA      = "ToolChefs_MP_drawTimeDelta"
    FRAME_DELTA     = "ToolChefs_MP_frameInterval"
    SHOW_ROTATION   = "ToolChefs_MP_showRotationKeyFrames"
    FNUMBER_COLOR   = "ToolChefs_MP_frameNumberColor"
    SHOW_KNUMBER    = "ToolChefs_MP_showKeyNumbers"
    SHOW_FNUMBER    = "ToolChefs_MP_showFrameNumbers"
    ALTERNATE_COLOR = "ToolChefs_MP_alternatePathColor"
    USEPIVOTS       = "ToolChefs_MP_usePivots"

_default_values = {
                StaticLabels.FRAMES_BEFORE	: 20,
                StaticLabels.FRAMES_AFTER	: 20,
                StaticLabels.PATH_COLOR		: [0, 178, 238],
                StaticLabels.WPATH_COLOR	: [255, 0, 0],
                StaticLabels.WTANGENT_COLOR : [139, 0, 0],
                StaticLabels.CFRAME_COLOR	: [255, 255, 0],
                StaticLabels.TANGENT_COLOR	: [139, 105, 105],
                StaticLabels.BTANGENT_COLOR	: [139, 69, 19],
                StaticLabels.BPATH_COLOR	: [51, 51, 51],
                StaticLabels.FNUMBER_COLOR	: [25, 25, 25],
                StaticLabels.PATH_SIZE		: 3,
                StaticLabels.FRAME_SIZE		: 7,
                StaticLabels.SHOW_PATH		: True,
                StaticLabels.SHOW_TANGENTS	: False,
                StaticLabels.SHOW_KEYFRAMES	: True,
                StaticLabels.SHOW_ROTATION	: True,
                StaticLabels.TIME_DELTA		: 0.1,
                StaticLabels.FRAME_DELTA	: 5,
                StaticLabels.SHOW_KNUMBER	: False,
                StaticLabels.SHOW_FNUMBER	: False,
                StaticLabels.ALTERNATE_COLOR: False,
                StaticLabels.USEPIVOTS		: False
                }

class TOOLS:
    EDIT    = 'motionPathEdit'
    DRAW    = 'motionPathDraw'

    @classmethod
    def all(cls):
        return [cls.EDIT, cls.DRAW]

    @classmethod
    def tooltip(cls, name):
        if (name == cls.EDIT):
            return 'Left-Click: Select/Move\nShift+Left-Click: Add to selection\nCTRL+Left-Click: Toggle selection\nCTRL+Left-Click-Drag: Move Selection on the XY plane\nCTRL+Middle-Click-Drag: Move Along Y Axis\nRight-Click on path/frame/key: show menu'
        elif (name == cls.DRAW):
            return 'Left-Click key frame then drag to draw path\nCTRL-Left-Click key frame then drag to draw proximity stroke\nMiddle-Click in the viewport to add a keyframe at the current time.'

script_job_ids = []

#############################################################
### FUNCTIONS ###############################################
#############################################################

def _maya_api_version():
    return int(cmds.about(api=True))

def _get_icons_path():
    tokens = __file__.split(os.path.sep)
    return os.path.sep.join(tokens[:-1] + ['icons']) + os.path.sep

def _delete_script_jobs():
    global script_job_ids
    for id in script_job_ids:
        if cmds.scriptJob(exists=id):
            cmds.scriptJob(kill=id)

    script_job_ids = []

def _playback_range():
    min = cmds.playbackOptions(q=True, minTime=True)
    max = cmds.playbackOptions(q=True, maxTime=True)

    cmds.tcMotionPathCmd(frameRange=[min, max])
    cmds.tcMotionPathCmd(refreshdt=True)
    cmds.refresh()

def _delete_all_event():
    if tc_motion_path_widget is None:
        return

    tc_motion_path_widget.empty_buffer_paths()

def	_script_job(callback, event = None, conditionChange=None, compressUndo=False):
    global script_job_ids

    id = None
    if not conditionChange is None:
        id = cmds.scriptJob(conditionChange=[conditionChange, callback])
    elif not event is None:
        id = cmds.scriptJob(event=[event, callback])

    if id:
        script_job_ids.append(id)

def _init_script_jobs():
    _script_job(_playback_range, event="playbackRangeChanged")
    _script_job(_tool_changed, event="ToolChanged")
    _script_job(_delete_all_event, event="deleteAll")

def _tool_changed():
    if tc_motion_path_widget is None:
        return

    tool = cmds.currentCtx()
    tc_motion_path_widget.set_active_tool(tool)

def _disable():
    _delete_script_jobs()

    for t in TOOLS.all():
        if cmds.currentCtx() == t:
            cmds.setToolTo('selectSuperContext')
        if cmds.contextInfo(t, exists=True):
            cmds.deleteUI(t)

    tc_motion_path_widget.set_active_tool('selectSuperContext')

def _init_tool():
    if not cmds.contextInfo(TOOLS.EDIT, exists=True):
        cmds.tcMotionPathEditContext(TOOLS.EDIT)
    if not cmds.contextInfo(TOOLS.DRAW, exists=True):
        cmds.tcMotionPathDrawContext(TOOLS.DRAW)

    _init_script_jobs()

def _enable_motion_path(status):
    if (status == False):
        _disable()
    else:
        _init_tool()
        _refresh_selection()
        _playback_range()

    cmds.tcMotionPathCmd(enable=status)
    for p in cmds.getPanel(type="modelPanel"):
        cmds.modelEditor(p, edit=True, rnm="vp2Renderer",
                         rom="ToolchefsMotionPathOverride")

def _refresh_selection():
    sl = cmds.ls(sl=True, l=True)
    cmds.select(*sl, r=True)

def _get_default_value(name):
    if cmds.optionVar(exists = name):
        return cmds.optionVar(q = name)

    return _default_values[name]

def _set_default_value(name, value):
    if isinstance(value, int) or isinstance(value, bool):
        cmds.optionVar(intValue = [name, value])
    elif isinstance(value, float):
        cmds.optionVar(floatValue = [name, value])
    elif isinstance(value, list):
        cmds.optionVar(clearArray = name)
        for v in value:
            cmds.optionVar(floatValueAppend = [name, v])

#############################################################
### GUI #####################################################
#############################################################

def _build_layout(horizontal):
    if (horizontal):
        layout = QtWidgets.QHBoxLayout()
    else:
        layout = QtWidgets.QVBoxLayout()
    layout.setContentsMargins(QtCore.QMargins(2, 2, 2, 2))
    return layout

def create_separator(vertical=False):
    separator = QtWidgets.QFrame()
    if vertical:
        separator.setFrameShape(QtWidgets.QFrame.VLine)
    else:
        separator.setFrameShape(QtWidgets.QFrame.HLine)
    separator.setFrameShadow(QtWidgets.QFrame.Sunken)
    return separator

class LineWidget(QtWidgets.QWidget):
    def __init__(self, label, widget, width=110, parent=None):
        super(LineWidget, self).__init__(parent=parent)

        self._main_layout = _build_layout(True)
        self.setLayout(self._main_layout)

        label = QtWidgets.QLabel(label)
        label.setFixedWidth(width)
        self._main_layout.addWidget(label)
        self._main_layout.addWidget(widget)


class MotionPathBufferPaths(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(MotionPathBufferPaths, self).__init__(parent=parent)

        self._last_selection = []

        self._main_layout = _build_layout(True)
        self.setLayout(self._main_layout)

        self._create_widgets()
        self._connect_signals()

    def _create_widgets(self):
        self._buffer_paths = QtWidgets.QTreeWidget()
        self._buffer_paths.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self._buffer_paths.setHeaderLabels(["Name"])
        self._main_layout.addWidget(self._buffer_paths)

        v_layout = _build_layout(False)
        self._main_layout.addLayout(v_layout)

        self._add_button = QtWidgets.QPushButton("Buffer Paths")
        self._delete_selected = QtWidgets.QPushButton("Delete Selected")
        self._delete_all = QtWidgets.QPushButton("Delete All")
        self._convert_selected = QtWidgets.QPushButton("Convert Selected to Nurbs Curve")

        self._delete_selected.setEnabled(False)
        self._convert_selected.setEnabled(False)

        v_layout.addWidget(self._add_button, stretch=0, alignment=QtCore.Qt.AlignTop)
        v_layout.addWidget(self._delete_all, stretch=0, alignment=QtCore.Qt.AlignTop)
        v_layout.addWidget(self._delete_selected, stretch=0, alignment=QtCore.Qt.AlignTop)
        v_layout.addWidget(self._convert_selected, stretch=0, alignment=QtCore.Qt.AlignTop)
        v_layout.addWidget(QtWidgets.QWidget(), stretch=1)

    def _connect_signals(self):
        self._add_button.clicked.connect(self._add_button_clicked)
        self._delete_all.clicked.connect(self._delete_all_clicked)
        self._delete_selected.clicked.connect(self._delete_selected_clicked)
        self._convert_selected.clicked.connect(self._convert_selected_clicked)

        self._buffer_paths.itemSelectionChanged.connect(self._item_selection_changed)

    def _add_button_clicked(self):
        cmds.tcMotionPathCmd(addBufferPaths=True)
        for sel in cmds.ls(sl=True):
            item = QtWidgets.QTreeWidgetItem()
            item.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsSelectable)
            item.setText(0, sel)
            self._buffer_paths.addTopLevelItem(item)
        cmds.refresh()

    def _delete_selected_clicked(self):
        items = self._buffer_paths.selectedItems()
        for item in items:
            index = self._buffer_paths.indexFromItem(item)
            cmds.tcMotionPathCmd(deleteBufferPathAtIndex=index.row())
            self._buffer_paths.takeTopLevelItem(index.row())
        cmds.refresh()

    def _delete_all_clicked(self):
        self._buffer_paths.clear()
        cmds.tcMotionPathCmd(deleteAllBufferPaths=True)
        cmds.refresh()

    def _convert_selected_clicked(self):
        items = self._buffer_paths.selectedItems()
        cmds.undoInfo(openChunk=True, chunkName="tcMotionPathDeleteBufferPaths")
        try:
            for item in items:
                index = self._buffer_paths.indexFromItem(item)
                cmds.tcMotionPathCmd(convertBufferPath=index.row())
        except:
            traceback.print_exc(file=sys.stdout)
        cmds.undoInfo(closeChunk=True)

    def empty_buffer_paths(self):
        self._last_selection = []
        self._buffer_paths.clear()
        cmds.refresh()

    def deselect_all(self):
        for s in self._last_selection:
            cmds.tcMotionPathCmd(deselectBufferPathAtIndex=s)
            item = self._buffer_paths.topLevelItem(s)
            self._buffer_paths.setItemSelected(item, False)

    def _refresh_button_states(self, value):
        self._delete_selected.setEnabled(value)
        self._convert_selected.setEnabled(value)

    def _item_selection_changed(self):
        for s in self._last_selection:
            cmds.tcMotionPathCmd(deselectBufferPathAtIndex=s)

        self._last_selection = []
        items = self._buffer_paths.selectedItems()
        for item in items:
            index = self._buffer_paths.indexFromItem(item)
            cmds.tcMotionPathCmd(selectBufferPathAtIndex=index.row())
            self._last_selection.append(index.row())

        self._refresh_button_states(bool(items))

        cmds.refresh()

class EditOptions(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(EditOptions, self).__init__(parent=parent)

        self._main_layout = _build_layout(True)
        self.setLayout(self._main_layout)

        self._create_widgets()
        self._initialize_widgets()
        self._connect_signals()

    def _initialize_widgets(self):
        fb = _get_default_value(StaticLabels.FRAMES_BEFORE)
        cmds.tcMotionPathCmd(framesBefore=fb)
        self._frame_before.setValue(fb)

        fa = _get_default_value(StaticLabels.FRAMES_AFTER)
        cmds.tcMotionPathCmd(framesAfter=fa)
        self._frame_after.setValue(fa)

        skf = _get_default_value(StaticLabels.SHOW_KEYFRAMES)
        self._show_key_frames.setChecked(skf)

        sp = _get_default_value(StaticLabels.SHOW_PATH)
        self._show_path.setChecked(sp)

        st = _get_default_value(StaticLabels.SHOW_TANGENTS)
        self._show_tangents.setChecked(st)

        srf = _get_default_value(StaticLabels.SHOW_ROTATION)
        self._show_rotation.setChecked(srf)

        sfn = _get_default_value(StaticLabels.SHOW_FNUMBER)
        self._show_frame_number.setChecked(sfn)

        skn = _get_default_value(StaticLabels.SHOW_KNUMBER)
        self._show_key_number.setChecked(skn)

        ap = _get_default_value(StaticLabels.ALTERNATE_COLOR)
        self._alternating_color.setChecked(ap)

        up = _get_default_value(StaticLabels.USEPIVOTS)
        self._use_pivots.setChecked(up)

        self._show_rotation_clicked()
        self._show_tangents_clicked()
        self._show_path_clicked()
        self._show_key_frames_clicked()
        self._show_frame_number_clicked()
        self._show_key_number_clicked()
        self._alternating_color_clicked()
        self._use_pivots_state_changed()

    def _get_selection_group_box(self):
        groupBox = QtWidgets.QGroupBox("Selection")
        vl = _build_layout(False)
        groupBox.setLayout(vl)

        self._lock_selection = QtWidgets.QPushButton()
        self._lock_selection.setCheckable(True)
        self._lock_selection.setText("Lock selection")
        vl.addWidget(self._lock_selection)

        hl = _build_layout(True)
        vl.addLayout(hl)

        self._interactive_locked_selection = QtWidgets.QCheckBox("Interactive")
        self._interactive_locked_selection.setChecked(True)
        self._interactive_locked_selection.setEnabled(False)
        hl.addWidget(self._interactive_locked_selection)

        self._refresh_locked_selection_button = QtWidgets.QPushButton()
        self._refresh_locked_selection_button.setText("Refresh")
        self._refresh_locked_selection_button.setEnabled(False)
        hl.addWidget(self._refresh_locked_selection_button)

        return groupBox

    def _get_pivots_group_box(self):
        groupBox = QtWidgets.QGroupBox("Pivots")
        vl = _build_layout(False)
        groupBox.setLayout(vl)

        self._use_pivots = QtWidgets.QCheckBox("Use Pivots")
        self._use_pivots.setChecked(True)
        vl.addWidget(self._use_pivots)

        return groupBox

    def _create_widgets(self):
        vl = _build_layout(False)
        self._main_layout.addLayout(vl)

        self._frame_before = QtWidgets.QSpinBox(parent=self)
        self._frame_before.setSingleStep(1)
        self._frame_before.setMinimum(0)
        vl.addWidget(LineWidget("Frame before:", self._frame_before, parent=self))

        self._frame_after = QtWidgets.QSpinBox(parent=self)
        self._frame_after.setSingleStep(1)
        self._frame_after.setMinimum(0)
        vl.addWidget(LineWidget("Frame after:", self._frame_after, parent=self))

        vl.addWidget(self._get_selection_group_box())
        vl.addWidget(self._get_pivots_group_box())

        self._main_layout.addWidget(create_separator(True))

        gl = QtWidgets.QGridLayout()
        gl.setContentsMargins(QtCore.QMargins(2, 2, 2, 2))
        self._main_layout.addLayout(gl)

        self._show_key_frames = QtWidgets.QPushButton()
        self._show_key_frames.setCheckable(True)
        self._show_key_frames.setFixedWidth(120)
        self._show_key_frames.setText("Show keyframes")
        gl.addWidget(self._show_key_frames, 0, 0)

        self._show_tangents = QtWidgets.QPushButton()
        self._show_tangents.setCheckable(True)
        self._show_tangents.setFixedWidth(120)
        self._show_tangents.setText("Show tangents")
        gl.addWidget(self._show_tangents, 1, 0)

        self._show_key_number = QtWidgets.QPushButton()
        self._show_key_number.setCheckable(True)
        self._show_key_number.setFixedWidth(120)
        self._show_key_number.setText("Show key numbers")
        gl.addWidget(self._show_key_number, 2, 0)

        self._show_rotation = QtWidgets.QPushButton()
        self._show_rotation.setCheckable(True)
        self._show_rotation.setFixedWidth(120)
        self._show_rotation.setText("Show rotation keys")
        gl.addWidget(self._show_rotation, 3, 0)

        self._show_path = QtWidgets.QPushButton()
        self._show_path.setCheckable(True)
        self._show_path.setFixedWidth(120)
        self._show_path.setText("Show path")
        gl.addWidget(self._show_path, 0, 1)

        self._alternating_color = QtWidgets.QPushButton()
        self._alternating_color.setCheckable(True)
        self._alternating_color.setFixedWidth(120)
        self._alternating_color.setText("Alternate path color")
        gl.addWidget(self._alternating_color, 1, 1)

        self._show_frame_number = QtWidgets.QPushButton()
        self._show_frame_number.setCheckable(True)
        self._show_frame_number.setFixedWidth(120)
        self._show_frame_number.setText("Show frame numbers")
        gl.addWidget(self._show_frame_number, 2, 1)

    def _connect_signals(self):
        self._show_tangents.clicked.connect(self._show_tangents_clicked)
        self._lock_selection.clicked.connect(self._lock_selection_clicked)
        self._show_path.clicked.connect(self._show_path_clicked)
        self._show_rotation.clicked.connect(self._show_rotation_clicked)
        self._show_key_frames.clicked.connect(self._show_key_frames_clicked)
        self._show_frame_number.clicked.connect(self._show_frame_number_clicked)
        self._show_key_number.clicked.connect(self._show_key_number_clicked)
        self._alternating_color.clicked.connect(self._alternating_color_clicked)

        self._interactive_locked_selection.stateChanged.connect(
                            self._interactive_locked_selection_state_changed)
        self._refresh_locked_selection_button.clicked.connect(
                            self._refresh_locked_selection_button_clicked)

        self._use_pivots.stateChanged.connect(self._use_pivots_state_changed)

        self._frame_after.valueChanged.connect(self._frame_after_changed)
        self._frame_before.valueChanged.connect(self._frame_before_changed)

    def _frame_after_changed(self, value):
        cmds.tcMotionPathCmd(framesAfter=value)
        cmds.tcMotionPathCmd(refreshdt=True)
        _set_default_value(StaticLabels.FRAMES_AFTER, value)
        cmds.refresh()

    def _frame_before_changed(self, value):
        cmds.tcMotionPathCmd(framesBefore=value)
        cmds.tcMotionPathCmd(refreshdt=True)
        _set_default_value(StaticLabels.FRAMES_BEFORE, value)
        cmds.refresh()

    def _interactive_locked_selection_state_changed(self, value):
        value = self._interactive_locked_selection.isChecked()
        cmds.tcMotionPathCmd(lockedModeInteractive=value)

        self._refresh_locked_selection_button.setEnabled(not value)

        cmds.refresh()

    def _use_pivots_state_changed(self):
        value = self._use_pivots.isChecked()

        cmds.tcMotionPathCmd(usePivots=value)
        cmds.tcMotionPathCmd(refreshdt=True)

        _set_default_value(StaticLabels.USEPIVOTS, value)

        cmds.refresh()

    def _refresh_locked_selection_button_clicked(self):
        cmds.tcMotionPathCmd(refreshLockedSelection=True)
        cmds.refresh()

    def _show_frame_number_clicked(self):
        is_checked = self._show_frame_number.isChecked()
        if is_checked:
            self._show_frame_number.setText("Hide frame numbers")
        else:
            self._show_frame_number.setText("Show frame numbers")

        cmds.tcMotionPathCmd(showFrameNumbers=is_checked)
        _set_default_value(StaticLabels.SHOW_FNUMBER, is_checked)
        cmds.refresh()

    def _alternating_color_clicked(self):
        is_checked = self._alternating_color.isChecked()
        if is_checked:
            self._alternating_color.setText("Uniform path color")
        else:
            self._alternating_color.setText("Alternate path color")

        cmds.tcMotionPathCmd(alternatingFrames=is_checked)
        _set_default_value(StaticLabels.ALTERNATE_COLOR, is_checked)
        cmds.refresh()

    def _show_key_number_clicked(self):
        is_checked = self._show_key_number.isChecked()
        if is_checked:
            self._show_key_number.setText("Hide key numbers")
        else:
            self._show_key_number.setText("Show key numbers")

        cmds.tcMotionPathCmd(showKeyFrameNumbers=is_checked)
        _set_default_value(StaticLabels.SHOW_KNUMBER, is_checked)
        cmds.refresh()

    def _show_key_frames_clicked(self):
        is_checked = self._show_key_frames.isChecked()
        if is_checked:
            self._show_key_frames.setText("Hide keyframes")
        else:
            self._show_key_frames.setText("Show keyframes")

        self._show_tangents.setEnabled(is_checked)
        self._show_rotation.setEnabled(is_checked)
        self._show_key_number.setEnabled(is_checked)
        cmds.tcMotionPathCmd(showKeyFrames=is_checked)
        _set_default_value(StaticLabels.SHOW_KEYFRAMES, is_checked)
        cmds.refresh()

    def _show_path_clicked(self):
        is_checked = self._show_path.isChecked()
        if is_checked:
            self._show_path.setText("Hide path")
        else:
            self._show_path.setText("Show path")

        self._alternating_color.setEnabled(is_checked)
        cmds.tcMotionPathCmd(showPath=self._show_path.isChecked())
        _set_default_value(StaticLabels.SHOW_PATH, self._show_path.isChecked())
        cmds.refresh()

    def _show_rotation_clicked(self):
        if self._show_rotation.isChecked():
            self._show_rotation.setText("Hide rotation keys")
        else:
            self._show_rotation.setText("Show rotation keys")

        cmds.tcMotionPathCmd(showRotationKeyFrames=self._show_rotation.isChecked())
        _set_default_value(StaticLabels.SHOW_ROTATION, self._show_rotation.isChecked())
        cmds.refresh()

    def _show_tangents_clicked(self):
        if self._show_tangents.isChecked():
            self._show_tangents.setText("Hide tangents")
        else:
            self._show_tangents.setText("Show tangents")

        cmds.tcMotionPathCmd(showTangents=self._show_tangents.isChecked())
        _set_default_value(StaticLabels.SHOW_TANGENTS, self._show_tangents.isChecked())
        cmds.refresh()

    def _lock_selection_clicked(self):
        is_checked = self._lock_selection.isChecked()

        if is_checked:
            self._lock_selection.setText("Unlock selection")
            cmds.tcMotionPathCmd(lockedMode=True)
        else:
            self._lock_selection.setText("Lock selection")
            cmds.tcMotionPathCmd(lockedMode=False)

            _refresh_selection()

        self._interactive_locked_selection.setEnabled(is_checked)
        self._refresh_locked_selection_button.setEnabled(is_checked and
                            not self._interactive_locked_selection.isChecked())

    def _uncheck_lock_selection_button(self):
        self._lock_selection.setChecked(False)
        self._lock_selection_clicked()


class EditButton(QtWidgets.QPushButton):
    def __init__(self, icon_path, maya_tool_name, parent=None):
        super(EditButton, self).__init__(parent=parent)
        self.setFixedSize(40, 40)
        self.setCheckable(True)
        self.setToolTip(TOOLS.tooltip(maya_tool_name))

        self.setIcon(QtGui.QIcon(icon_path))
        self.setIconSize(QtCore.QSize(38, 38))
        self._maya_tool_name = maya_tool_name
        self._previous_context = cmds.currentCtx()

        self.clicked.connect(self._clicked)

    def _clicked(self):
        if self.isChecked():
            self._previous_context = cmds.currentCtx()
            cmds.setToolTo(self._maya_tool_name)
        else:
            cmds.setToolTo(self._previous_context)

    def get_maya_tool_name(self):
        return self._maya_tool_name

    maya_tool_name = property(get_maya_tool_name)


class DrawButtonOptions(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(DrawButtonOptions, self).__init__(parent=parent)

        self._main_layout = _build_layout(False)
        self.setLayout(self._main_layout)

        self._create_widgets()
        self._connect_signals()
        self._initialize_widgets()

    def _create_widgets(self):
        self._frame_value = QtWidgets.QSpinBox(parent=self)
        self._frame_value.setSingleStep(1)
        self._frame_value.setMinimum(0)
        self._add_line_widget('Frame Interval:', self._frame_value)

        self._draw_value = QtWidgets.QDoubleSpinBox(parent=self)
        self._draw_value.setSingleStep(0.1)
        self._add_line_widget('Draw Time Interval (secs):', self._draw_value)

        self._mode_group = QtWidgets.QButtonGroup(parent=self)
        self._closest_mode = QtWidgets.QRadioButton("Closest")
        self._closest_mode.setChecked(True)
        self._mode_group.addButton(self._closest_mode)

        self._spread_mode = QtWidgets.QRadioButton("Spread")
        self._mode_group.addButton(self._spread_mode)

        w = QtWidgets.QWidget()
        hl = _build_layout(True)
        w.setLayout(hl)

        hl.addWidget(self._closest_mode)
        hl.addWidget(self._spread_mode)

        self._add_line_widget('Stroke Mode:', w)

    def _initialize_widgets(self):
        value = _get_default_value(StaticLabels.TIME_DELTA)
        self._draw_value.setValue(value)
        cmds.tcMotionPathCmd(drawTimeInterval=value)

        value = _get_default_value(StaticLabels.FRAME_DELTA)
        self._frame_value.setValue(value)
        cmds.tcMotionPathCmd(frameInterval=value)

        self.setEnabled(False)

    def _add_line_widget(self, label, widget):
        w = LineWidget(label, widget, width=140, parent=self)
        self._main_layout.addWidget(w)

    def _draw_value_changed(self, value):
        cmds.tcMotionPathCmd(drawTimeInterval=value)
        _set_default_value(StaticLabels.TIME_DELTA, value)

    def _frame_value_changed(self, value):
        cmds.tcMotionPathCmd(frameInterval=value)
        _set_default_value(StaticLabels.FRAME_DELTA, value)

    def _mode_group_changed(self, value):
        if self._closest_mode.isChecked():
            cmds.tcMotionPathCmd(strokeMode=0)
        else:
            cmds.tcMotionPathCmd(strokeMode=1)

    def _connect_signals(self):
        self._draw_value.valueChanged.connect(self._draw_value_changed)
        self._frame_value.valueChanged.connect(self._frame_value_changed)
        self._mode_group.buttonClicked.connect(self._mode_group_changed)


class EditButtons(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(EditButtons, self).__init__(parent=parent)

        self._buttons = []

        self._main_layout = _build_layout(False)
        self.setLayout(self._main_layout)

        self._create_widgets()
        self._initialize_widgets()

    def _create_widgets(self):
        icons_path = _get_icons_path()

        self._buttons.append(EditButton(icons_path+TOOLS.EDIT, TOOLS.EDIT))
        self._buttons.append(EditButton(icons_path+TOOLS.DRAW, TOOLS.DRAW))

        h_layout = _build_layout(True)
        h_layout.setSpacing(10)
        h_layout.addWidget(QtWidgets.QLabel("Tools:"), stretch=0, alignment=QtCore.Qt.AlignLeft)
        for w in self._buttons:
            h_layout.addWidget(w, stretch=0, alignment=QtCore.Qt.AlignLeft)
        h_layout.addWidget(QtWidgets.QWidget(), stretch=1)
        self._main_layout.addLayout(h_layout)

        self._main_layout.addWidget(create_separator())

        self._draw_button_options = DrawButtonOptions(self)
        self._main_layout.addWidget(self._draw_button_options)

    def refresh_enable_button_statuses(self):
        skf = _get_default_value(StaticLabels.SHOW_KEYFRAMES)
        for b in self._buttons:
            b.setEnabled(skf)

    def _initialize_widgets(self):
        self.refresh_enable_button_statuses()

    def set_active_tool(self, tool):
        for b in self._buttons:
            if b.maya_tool_name != tool and b.isChecked():
                b.setChecked(False)
            elif b.maya_tool_name == tool and not b.isChecked():
                b.setChecked(True)

        self._draw_button_options.setEnabled(TOOLS.DRAW == tool)

class EditPathWidget(QtWidgets.QWidget):
    enabled = QtCore.Signal(bool)

    def __init__(self, parent=None):
        super(EditPathWidget, self).__init__(parent=parent)

        self._main_layout = _build_layout(False)
        self.setLayout(self._main_layout)

        self._create_widgets()
        self._initialize_widgets()
        self._connect_signals()

    def _create_widgets(self):
        self._enable_button = QtWidgets.QPushButton()
        self._enable_button.setText("Enable")
        self._enable_button.setCheckable(True)
        self._main_layout.addWidget(self._enable_button)

        self._main_layout.addWidget(create_separator())

        self._edit_options = EditOptions()
        self._main_layout.addWidget(self._edit_options)

        self._main_layout.addWidget(create_separator())

        self._draw_mode_group = QtWidgets.QButtonGroup(parent=self)
        self._world_mode = QtWidgets.QRadioButton("World Space")
        self._world_mode.setChecked(True)
        self._draw_mode_group.addButton(self._world_mode)

        self._camera_mode = QtWidgets.QRadioButton("Camera Space")
        self._draw_mode_group.addButton(self._camera_mode)

        w = QtWidgets.QWidget()
        hl = _build_layout(True)
        w.setLayout(hl)

        hl.addWidget(self._world_mode)
        hl.addWidget(self._camera_mode)

        self._draw_mode_widget = LineWidget('Draw Mode:', w, parent=self)
        self._main_layout.addWidget(self._draw_mode_widget)

        self._main_layout.addWidget(create_separator())

        self._edit_buttons = EditButtons()
        self._main_layout.addWidget(self._edit_buttons)

        self._main_layout.addWidget(QtWidgets.QWidget(), stretch=1)

    def _initialize_widgets(self):
        self._enable_button.setChecked(True)

    def _connect_signals(self):
        self._enable_button.clicked.connect(self._enable_button_clicked)
        self._edit_options._show_key_frames.clicked.connect(self._edit_buttons.refresh_enable_button_statuses)
        self._draw_mode_group.buttonClicked.connect(self._draw_mode_changed)

    def _draw_mode_changed(self, value):
        if self._world_mode.isChecked():
            cmds.tcMotionPathCmd(drawMode=0)
        else:
            cmds.tcMotionPathCmd(drawMode=1)
        cmds.refresh()

    def _enable_button_clicked(self):
        checked = self._enable_button.isChecked()
        if checked:
            self._enable_button.setText("Disable")
        else:
            self._enable_button.setText("Enable")

        _enable_motion_path(checked)
        self._edit_buttons.setEnabled(checked)

        self._edit_options._uncheck_lock_selection_button()
        self._edit_options.setEnabled(checked)

        self._draw_mode_widget.setEnabled(checked)

        self.enabled.emit(checked)
        cmds.refresh()


class SettingsColorButton(QtWidgets.QPushButton):
    color_changed = QtCore.Signal()

    def __init__(self, color, parent=None):
        super(SettingsColorButton, self).__init__(parent=parent)

        self._color = None
        self._set_color(color)
        self.clicked.connect(self._on_color_picker)

    def _set_color(self, color):
        if color != self._color:
            self._color = color
            self.color_changed.emit()

        if self._color:
            self.setStyleSheet("QPushButton {background-color: %s}" %
                               self._color.name())
        else:
            self.setStyleSheet("")

    def _get_color(self):
        return self._color

    color = property(_get_color, _set_color)

    def _on_color_picker(self):
        dlg = QtWidgets.QColorDialog()
        if self._color:
            dlg.setCurrentColor(QtGui.QColor(self._color))

        if dlg.exec_():
            self._set_color(dlg.currentColor())

class OpenGLLinedEdit(QtWidgets.QLineEdit):
    BOTTOM_SIZE = 0.5
    TOP_SIZE = 20

    def __init__(self, parent=None):
        super(OpenGLLinedEdit, self).__init__(parent=parent)
        validator = QtGui.QDoubleValidator()
        validator.setBottom(self.BOTTOM_SIZE)
        validator.setDecimals(2)
        self.setValidator(validator)

    def focusOutEvent(self, event):
        super(OpenGLLinedEdit, self).focusOutEvent(event)

        value = self.text()
        if value < self.BOTTOM_SIZE:
            self.setText(str(self.BOTTOM_SIZE))


class MotionPathWidgetSettings(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(MotionPathWidgetSettings, self).__init__(parent=parent)

        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Preferred)

        self._main_layout = _build_layout(False)
        self.setLayout(self._main_layout)

        self._create_widgets()
        self._initialize_widgets()
        self._connect_signals()

    def _create_widgets(self):

        about = QtWidgets.QLabel()
        about.setPixmap(QtGui.QPixmap(_get_icons_path()+'abouts.png'))
        self._main_layout.addWidget(about)

        self._main_layout.addWidget(create_separator())

        self._path_size = OpenGLLinedEdit(parent=self)
        self._add_line_widget('Path Size:', self._path_size)

        self._main_layout.addWidget(create_separator())

        self._frame_size = OpenGLLinedEdit(parent=self)
        self._add_line_widget('Frames Size:', self._frame_size)

        self._main_layout.addWidget(create_separator())

        self._path_color = SettingsColorButton(QtGui.QColor(0, 0, 255, 255),
                                               parent=self)
        self._add_line_widget('Path Color:', self._path_color)

        self._cframe_color = SettingsColorButton(QtGui.QColor(255, 0, 0, 255), parent=self)
        self._add_line_widget('Current Frame Color:', self._cframe_color)

        self._tangent_color = SettingsColorButton(QtGui.QColor(255, 0, 0, 255), parent=self)
        self._add_line_widget('Tangent Color:', self._tangent_color)

        self._broken_tangent_color = SettingsColorButton(QtGui.QColor(255, 0, 0, 255), parent=self)
        self._add_line_widget('Broken Tangent Color:', self._broken_tangent_color)

        self._buffer_path_color = SettingsColorButton(QtGui.QColor(255, 0, 0, 255), parent=self)
        self._add_line_widget('Buffer Path Color:', self._buffer_path_color)

        self._weighted_path_color = SettingsColorButton(QtGui.QColor(255, 50, 50, 255),
                                                    parent=self)
        self._add_line_widget('Weighted Path Color:', self._weighted_path_color)

        self._weighted_tangent_color = SettingsColorButton(QtGui.QColor(255, 50, 50, 255),
                                                           parent=self)
        self._add_line_widget('Weighted Path Tangent Color:', self._weighted_tangent_color)

        self._frame_number_color = SettingsColorButton(QtGui.QColor(25, 25, 25, 255), parent=self)
        self._add_line_widget('Frame Number Color:', self._frame_number_color)

        self._main_layout.addWidget(QtWidgets.QWidget(), stretch=1)

    def _initialize_color_widget(self, widget, g_var_name, cmd_arg_name):
        color = _get_default_value(g_var_name)
        widget.color = QtGui.QColor(color[0], color[1], color[2], 255)
        c_float = [float(c)/255.0 for c in color]
        cmds.tcMotionPathCmd(**{cmd_arg_name:[c_float[0], c_float[1], c_float[2]]})

    def _initialize_widgets(self):
        ps = _get_default_value(StaticLabels.PATH_SIZE)
        self._path_size.setText(str(ps))
        cmds.tcMotionPathCmd(pathSize=ps)

        fs = _get_default_value(StaticLabels.FRAME_SIZE)
        self._frame_size.setText(str(fs))
        cmds.tcMotionPathCmd(frameSize=fs)

        self._initialize_color_widget(self._path_color, StaticLabels.PATH_COLOR, 'pathColor')
        self._initialize_color_widget(self._weighted_path_color, StaticLabels.WPATH_COLOR, 'weightedPathColor')
        self._initialize_color_widget(self._weighted_tangent_color, StaticLabels.WTANGENT_COLOR, 'weightedPathTangentColor')
        self._initialize_color_widget(self._cframe_color, StaticLabels.CFRAME_COLOR, 'currentFrameColor')
        self._initialize_color_widget(self._tangent_color, StaticLabels.TANGENT_COLOR, 'tangentColor')
        self._initialize_color_widget(self._broken_tangent_color, StaticLabels.BTANGENT_COLOR, 'brokenTangentColor')
        self._initialize_color_widget(self._buffer_path_color, StaticLabels.BPATH_COLOR, 'bufferPathColor')
        self._initialize_color_widget(self._frame_number_color, StaticLabels.FNUMBER_COLOR, 'frameNumberColor')

        cmds.refresh()

    def _connect_signals(self):
        self._frame_size.editingFinished.connect(self._frame_size_changed)
        self._path_size.editingFinished.connect(self._path_size_changed)

        self._path_color.color_changed.connect(self._path_color_changed)
        self._weighted_path_color.color_changed.connect(self._weighted_path_color_changed)
        self._weighted_tangent_color.color_changed.connect(self._weighted_tangent_color_changed)
        self._cframe_color.color_changed.connect(self._cframe_color_changed)
        self._tangent_color.color_changed.connect(self._tangent_color_changed)
        self._broken_tangent_color.color_changed.connect(self._broken_tangent_color_changed)
        self._buffer_path_color.color_changed.connect(self._buffer_path_color_changed)
        self._frame_number_color.color_changed.connect(self._frame_number_color_changed)

    def _frame_size_changed(self):
        value = float(self._frame_size.text())

        cmds.tcMotionPathCmd(frameSize=value)
        _set_default_value(StaticLabels.FRAME_SIZE, value)
        cmds.refresh()

    def _path_size_changed(self):
        value = float(self._path_size.text())

        cmds.tcMotionPathCmd(pathSize=value)
        _set_default_value(StaticLabels.PATH_SIZE, value)
        cmds.refresh()

    def _widget_color_changed(self, color, g_var_name, cmd_arg_name):
        cmds.tcMotionPathCmd(**{cmd_arg_name:[float(color.red())/255.0, float(color.green())/255.0, float(color.blue())/255.0]})
        _set_default_value(g_var_name, [color.red(), color.green(), color.blue()])
        cmds.refresh()

    def _path_color_changed(self):
        self._widget_color_changed(self._path_color.color, StaticLabels.PATH_COLOR, 'pathColor')

    def _weighted_tangent_color_changed(self):
        self._widget_color_changed(self._weighted_tangent_color.color, StaticLabels.WTANGENT_COLOR, 'weightedPathTangentColor')

    def _weighted_path_color_changed(self):
        self._widget_color_changed(self._weighted_path_color.color, StaticLabels.WPATH_COLOR, 'weightedPathColor')

    def _cframe_color_changed(self):
        self._widget_color_changed(self._cframe_color.color, StaticLabels.CFRAME_COLOR, 'currentFrameColor')

    def _tangent_color_changed(self):
        self._widget_color_changed(self._tangent_color.color, StaticLabels.TANGENT_COLOR, 'tangentColor')

    def _broken_tangent_color_changed(self):
        self._widget_color_changed(self._broken_tangent_color.color, StaticLabels.BTANGENT_COLOR, 'brokenTangentColor')

    def _buffer_path_color_changed(self):
        self._widget_color_changed(self._buffer_path_color.color, StaticLabels.BPATH_COLOR, 'bufferPathColor')

    def _frame_number_color_changed(self):
        self._widget_color_changed(self._frame_number_color.color, StaticLabels.FNUMBER_COLOR, 'frameNumberColor')

    def _add_line_widget(self, label, widget):
        w = LineWidget(label, widget, parent=self)
        self._main_layout.addWidget(w)


class MotionPathWidget(MayaQWidgetDockableMixin, QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(MotionPathWidget, self).__init__(parent)
        self.setWindowTitle("Motion Path")
        self.setObjectName("ToolchefsMotionPath")

        self._main_layout = _build_layout(True)
        self.setLayout(self._main_layout)

        tabBar = QtWidgets.QTabBar(self)
        tabBar.setStyleSheet("QTabBar::tab{width:120px;}")
        tabBar.setExpanding(True)

        self.tabWidget = QtWidgets.QTabWidget(self)
        self.tabWidget.setTabBar(tabBar)
        self._main_layout.addWidget(self.tabWidget)

        scrollArea1 = QtWidgets.QScrollArea(self)
        scrollArea1.setWidgetResizable(True)
        self.mws = EditPathWidget()
        scrollArea1.setWidget(self.mws)
        self.tabWidget.addTab(scrollArea1, "Edit")

        scrollArea3 = QtWidgets.QScrollArea(self)
        scrollArea3.setWidgetResizable(True)
        self.mbp = MotionPathBufferPaths()
        scrollArea3.setWidget(self.mbp)
        self.tabWidget.addTab(scrollArea3, "Buffered Paths")

        scrollArea2 = QtWidgets.QScrollArea(self)
        scrollArea2.setWidgetResizable(True)
        mws = MotionPathWidgetSettings()
        scrollArea2.setWidget(mws)
        self.tabWidget.addTab(scrollArea2, "Settings")

        self.mws.enabled.connect(self._enabled)
        self.tabWidget.currentChanged.connect(self._tab_changed)

    def set_active_tool(self, tool):
        self.mws._edit_buttons.set_active_tool(tool)

    def _enabled(self, value):
        if (value == False):
            self.mbp.empty_buffer_paths()
        self.mbp.setEnabled(value)

    def enable_tool(self):
        if (self.mws._enable_button.isChecked()):
            _enable_motion_path(True)

    def _tab_changed(self, value):
        if self.tabWidget.tabText(value) != "Buffered Paths":
            self.mbp.deselect_all()

    def empty_buffer_paths(self):
        self.mbp.empty_buffer_paths()

    def closeEvent(self, event):
        _enable_motion_path(False)
        super(MotionPathWidget, self).closeEvent(event)


tc_motion_path_widget = None
def _get_dockable_motion_path_widget():
    global tc_motion_path_widget
    if tc_motion_path_widget is not None:
        try:
            if tc_motion_path_widget.isVisible():
                tc_motion_path_widget.close()
        except:
            pass

    tc_motion_path_widget = MotionPathWidget()
    return tc_motion_path_widget


def _open_gui():
    if cmds.workspaceControl(WCN, q=True, exists=True):
        cmds.workspaceControl(WCN, e=True, close=True)
        cmds.deleteUI(WCN, control=True)

    widget = _get_dockable_motion_path_widget()
    widget.show(dockable=True, area='right', floating=False, retain=True)
    cmds.workspaceControl(WCN, e=True, ttc=["AttributeEditor", -1],
                          wp="preferred", mw=420)

    widget.raise_()

    widget.enable_tool()

def run():
    if not cmds.pluginInfo('tcMotionPath', q=1, l=1):
        cmds.loadPlugin('tcMotionPath', quiet=True)

    _open_gui()
    _playback_range()
    _refresh_selection()



