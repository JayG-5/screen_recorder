import 'dart:ffi';
import 'dart:convert';
import 'package:ffi/ffi.dart';
import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';
import 'package:intl/intl.dart';

typedef GetMonitorListStrC = Pointer<Utf8> Function();
typedef GetMonitorListStrDart = Pointer<Utf8> Function();
typedef CreateScreenRecorderC = Pointer<Void> Function(Int32, Pointer<Utf8>);
typedef CreateScreenRecorderDart = Pointer<Void> Function(int, Pointer<Utf8>);
typedef StartRecordingC = Void Function(Pointer<Void>);
typedef StartRecordingDart = void Function(Pointer<Void>);
typedef StopRecordingC = Void Function(Pointer<Void>);
typedef StopRecordingDart = void Function(Pointer<Void>);

class MonitorInfo {
  final int index;
  final String name;
  final int width;
  final int height;

  MonitorInfo(this.index, this.name, this.width, this.height);

  @override
  String toString() {
    return 'MonitorInfo{index: $index, name: $name, width: $width, height: $height}';
  }
}

class MonitorService {
  final DynamicLibrary _lib;

  MonitorService() : _lib = DynamicLibrary.open('screen_recorder.dll');

  List<MonitorInfo> getMonitorList() {
    final getMonitorListStr = _lib
        .lookupFunction<GetMonitorListStrC, GetMonitorListStrDart>('GetMonitorListStr');
    final Pointer<Utf8> monitorListStrPtr = getMonitorListStr();
    final monitorListStr = monitorListStrPtr.toDartString();
    calloc.free(monitorListStrPtr);

    final monitors = <MonitorInfo>[];
    final monitorEntries = monitorListStr.split(';');
    for (var entry in monitorEntries) {
      if (entry.isNotEmpty) {
        final fields = entry.split(',');
        final index = int.parse(fields[0]);
        final name = fields[1];
        final width = int.parse(fields[2]);
        final height = int.parse(fields[3]);
        monitors.add(MonitorInfo(index, name, width, height));
      }
    }
    return monitors;
  }
}

class ScreenRecorderService {
  late Pointer<Void> _recorder;
  final DynamicLibrary _lib;

  ScreenRecorderService() : _lib = DynamicLibrary.open('screen_recorder.dll');

  void createRecorder(int monitorIndex, String outputFilePath) {
    final createScreenRecorder = _lib.lookupFunction<CreateScreenRecorderC,
        CreateScreenRecorderDart>('CreateScreenRecorder');
    _recorder = createScreenRecorder(monitorIndex, outputFilePath.toNativeUtf8());
  }

  void startRecording() {
    final startRecording = _lib.lookupFunction<StartRecordingC, StartRecordingDart>('StartRecording');
    startRecording(_recorder);
  }

  void stopRecording() {
    final stopRecording = _lib.lookupFunction<StopRecordingC, StopRecordingDart>('StopRecording');
    stopRecording(_recorder);
  }
}
