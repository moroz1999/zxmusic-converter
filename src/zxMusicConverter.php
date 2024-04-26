<?php

class zxMusicConverter
{
    protected $originalFilePath;
    protected $originalFileName;
    protected $baseName;

    /**
     * @param mixed $baseName
     */
    public function setBaseName($baseName)
    {
        $this->baseName = $baseName;
    }


    protected $converterPath;
    protected $resultPath;
    /**
     * @var
     * 0- AY, 1- YM
     */
    protected $chipType = 0;
    /**
     * @var
     * 0-ABC, 1-ACB, 2-BAC, 3-BCA, 4-CBA, 5-CAB
     */
    protected $channels = 1;
    protected $frequency = 1750000;
    protected $frameDuration = 20000;

    /**
     * @param int $frameDuration
     */
    public function setFrameDuration($frameDuration)
    {
        $this->frameDuration = $frameDuration;
    }

    /**
     * @param mixed $channels
     */
    public function setChannels($channels)
    {
        $this->channels = $channels;
    }


    /**
     * @param mixed $frequency
     */
    public function setFrequency($frequency)
    {
        $this->frequency = $frequency;
    }

    /**
     * @param mixed $originalFilePath
     */
    public function setOriginalFilePath($originalFilePath)
    {
        $this->originalFilePath = $originalFilePath;
    }

    /**
     * @param mixed $chipType
     */
    public function setChipType($chipType)
    {
        $this->chipType = $chipType;
    }

    public function setConverterPath($path)
    {
        $this->converterPath = $path;
    }

    public function setOriginalFileName($name)
    {
        $this->originalFileName = $name;
    }

    /**
     * @param mixed $resultPath
     */
    public function setResultPath($resultPath)
    {
        $this->resultPath = $resultPath;
    }

    public function convert()
    {
        $types = array('mp3');
        $result = false;
        if (is_file($this->originalFilePath) && $this->resultPath) {
            foreach ($types as &$convertedType) {
                $output = array();
                chdir($this->converterPath);
                $call = $this->converterPath . 'zxtune123.exe --quiet --core-options aym.interpolation=2,aym.clockrate=' . $this->frequency . ',aym.type=' . $this->chipType . ',aym.layout=' . $this->channels . ' --frameduration=' . $this->frameDuration . ' --' . $convertedType . ' filename="' . $this->resultPath . 'result[Subpath]",bitrate=320 "' . $this->originalFilePath . '" 2>&1';
                exec($call, $output);
            }
            $result = $this->parseInfo($output);
        }
        return $result;
    }

    protected function parseInfo($output)
    {
        $infoList = array();

        $info = array();
        foreach ($output as &$line) {
            if (preg_match('#' . preg_quote($this->originalFilePath, '#') . '(\?(.*))*#', $line, $matches)) {
                if ($info) {
                    $infoList[] = $info;
                }
                $info = array();
                if (isset($matches[2])) {
                    $extra = str_ireplace('/', '_', $matches[2]);
                    $info['convertedFile'] = $this->resultPath . 'result' . $extra;
                } else {
                    $info['convertedFile'] = $this->resultPath . 'result';
                }
            }
            if (preg_match('#Title:[\s]*(.*)#', $line, $matches)) {
                $info['title'] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
            if (preg_match('#Author:[\s]*(.*)#', $line, $matches)) {
                $info['author'] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
            if (preg_match('#Time:[\s]*([^\t]*)#', $line, $matches)) {
                $info['time'] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
            if (preg_match('#Channels:[\s]*(.*)#', $line, $matches)) {
                $info['channels'] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
            if (preg_match('#Type:[ ]*([^\t]*)#', $line, $matches)) {
                $info['type'] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
            if (preg_match('#Container:[ ]*([^\t]*)#', $line, $matches)) {
                $info['container'] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
            if (preg_match('#Program:[\s]*(.*)#', $line, $matches)) {
                $info['program'] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
        }
        if ($info) {
            $infoList[] = $info;
        }
        return $infoList;
    }

}

?>