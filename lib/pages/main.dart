import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:screen_recorder/controllers/app.dart';
import 'package:screen_recorder/services/capture.dart';

class MainPage extends StatelessWidget {
  MainPage({super.key});

  final AppController controller = Get.put(AppController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Screen Recorder')),
      body: Container(
        width: 300,
        height: 500,
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text('Select Monitor:', style: TextStyle(fontSize: 16)),
            Obx(() {
              return DropdownButton<MonitorInfo>(
                value: controller.selectedMonitor.value,
                onChanged: (MonitorInfo? newValue) {
                  if (newValue != null) {
                    controller.selectedMonitor.value = newValue;
                  }
                },
                items: controller.monitors.map((MonitorInfo monitor) {
                  return DropdownMenuItem<MonitorInfo>(
                    value: monitor,
                    child: Text(
                        '${monitor.name} (${monitor.width}x${monitor.height})'),
                  );
                }).toList(),
              );
            }),
            const SizedBox(height: 20),
            const Text('Save Path:', style: TextStyle(fontSize: 16)),
            Row(
              children: [
                Obx(() {
                  return Expanded(
                    child: Text(
                      controller.savePath.value.isNotEmpty
                          ? controller.savePath.value
                          : 'Select a directory',
                      style: const TextStyle(fontSize: 14, color: Colors.grey),
                    ),
                  );
                }),
                IconButton(
                  icon: const Icon(Icons.folder_open),
                  onPressed: () {
                    controller.selectSavePath();
                  },
                ),
              ],
            ),
            const SizedBox(height: 20),
            Center(
              child: Obx(() {
                return Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    IconButton(
                      icon: Icon(
                        Icons.play_arrow,
                        color: controller.isRecording.value
                            ? Colors.grey
                            : Colors.green,
                        size: 36,
                      ),
                      onPressed: controller.isRecording.value
                          ? null
                          : () {
                              controller.startRecording();
                            },
                    ),
                    const SizedBox(width: 20),
                    IconButton(
                      icon: Icon(
                        Icons.stop,
                        color: controller.isRecording.value
                            ? Colors.red
                            : Colors.grey,
                        size: 36,
                      ),
                      onPressed: controller.isRecording.value
                          ? () {
                              controller.stopRecording();
                            }
                          : null,
                    ),
                  ],
                );
              }),
            ),
          ],
        ),
      ),
    );
  }
}
