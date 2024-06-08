import 'package:get/get.dart';
import 'package:file_picker/file_picker.dart';
import 'package:screen_recorder/services/capture.dart';
import 'package:shared_preferences/shared_preferences.dart';

class AppController extends GetxController {
  final MonitorService monitorService = MonitorService();
  final ScreenRecorderService recorderService = ScreenRecorderService();

  var monitors = <MonitorInfo>[].obs;
  var selectedMonitor = MonitorInfo(0, '', 0, 0).obs;
  var savePath = ''.obs;
  var isRecording = false.obs;

  @override
  void onInit() {
    super.onInit();
    fetchMonitors();
    loadSavePath();
  }

  void fetchMonitors() {
    monitors.value = monitorService.getMonitorList();
    if (monitors.isNotEmpty) {
      selectedMonitor.value = monitors.first;
    }
  }

  Future<void> selectSavePath() async {
    String? selectedDirectory = await FilePicker.platform.getDirectoryPath();

    if (selectedDirectory != null) {
      savePath.value = selectedDirectory;
      await saveSavePath(selectedDirectory);
    } else {
      savePath.value = '';
    }
  }

  Future<void> saveSavePath(String path) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('savePath', path);
  }

  Future<void> loadSavePath() async {
    final prefs = await SharedPreferences.getInstance();
    savePath.value = prefs.getString('savePath') ?? '';
  }

  void startRecording() {
    if (savePath.value.isNotEmpty && selectedMonitor.value.index >= 0) {
      final filePath = '$savePath/${DateTime.now().toIso8601String()}.mp4';
      recorderService.createRecorder(selectedMonitor.value.index, filePath);
      recorderService.startRecording();
      isRecording.value = true;
    }
  }

  void stopRecording() {
    recorderService.stopRecording();
    isRecording.value = false;
  }
}
