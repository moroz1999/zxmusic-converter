<?php
include_once('zxMusicConverter.php');
if (isset($_POST['action'])) {
    $action = $_POST['action'];
    if ($action == 'upload') {
        $id = intval($_POST['id']);
        $channels = 1;
        if (isset($_POST['channels'])) {
            $channels = intval($_POST['channels']);
        }
        $chipType = 0;
        if (isset($_POST['chipType'])) {
            $chipType = intval($_POST['chipType']);
        }
        $frequency = false;
        if (isset($_POST['frequency'])) {
            $frequency = intval($_POST['frequency']);
        }
        $frameDuration = false;
        if (isset($_POST['frameDuration'])) {
            $frameDuration = intval($_POST['frameDuration']);
        }
        $baseName = false;
        if (isset($_POST['baseName'])) {
            $baseName = $_POST['baseName'];
        }

        if (isset($_FILES['original']['tmp_name'])) {
            $temporary = $_FILES['original']['tmp_name'];
            $workPath = RESULT_PATH . $id . '/';
            $originalFileName = 'originalfile';
            $originalFileFolder = UPLOADS_PATH . $id . '/';
            $originalFilePath = $originalFileFolder . $originalFileName;
            if (!is_dir(MUSIC_PATH)) {
                mkdir(MUSIC_PATH);
            }
            if (!is_dir($workPath)) {
                mkdir($workPath);
            }
            if (!is_dir($originalFileFolder)) {
                mkdir($originalFileFolder);
            }
            if (is_uploaded_file($temporary)) {
                move_uploaded_file($temporary, $originalFilePath);
                $converter = new zxMusicConverter();
                $converter->setBaseName($baseName);
                $converter->setOriginalFileName($originalFileName);
                $converter->setOriginalFilePath($originalFilePath);
                $converter->setChannels($channels);
                $converter->setChipType($chipType);
                if ($frequency) {
                    $converter->setFrequency($frequency);
                }
                if ($frameDuration) {
                    $converter->setFrameDuration($frameDuration);
                }
                $converter->setConverterPath(CONVERTER_PATH);
                $converter->setResultPath($workPath);
                if ($result = $converter->convert()) {
                    foreach ($result as $key => &$item) {
                        $item['id'] = $id;
                        $extra = '';
                        if (count($result) > 1) {
                            $extra = ' ' . ($key + 1);
                        }
                        $item['mp3Name'] = $baseName . $extra . '.mp3';
                        if (is_file($item['convertedFile'] . '.mp3')) {
                            rename($item['convertedFile'] . '.mp3', MUSIC_PATH . $baseName . $extra . '.mp3');
                        }
                        if (is_file($item['convertedFile'] . '.ogg')) {
                            rename($item['convertedFile'] . '.ogg', MUSIC_PATH . $baseName . $extra . '.ogg');
                        }
                    }
                }
                unlink($originalFilePath);
                rrmdir($workPath);
                rrmdir($originalFileFolder);

                $contentText = json_encode($result);

                $gzip_contents = gzencode($contentText, 3);
                header('Content-Type: application/json');
                header('Content-Encoding: gzip');
                header('Content-Length: ' . strlen($gzip_contents));
                echo $gzip_contents;
            }
        }


    }
}
function rrmdir($dir)
{
    if (is_dir($dir)) {
        $objects = scandir($dir);
        foreach ($objects as $object) {
            if ($object != "." && $object != "..") {
                if (filetype($dir . "/" . $object) == "dir") {
                    rmdir($dir . "/" . $object);
                } else {
                    unlink($dir . "/" . $object);
                }
            }
        }
        reset($objects);
        rmdir($dir);
    }
}