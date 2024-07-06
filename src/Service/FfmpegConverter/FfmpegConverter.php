<?php

namespace ZxMusic\Service\FfmpegConverter;

use Exception;

class FfmpegConverter
{
    public function __construct(
        private string $converterPath
    )
    {

    }

    /**
     * @throws Exception
     */
    public function convertToMp3(string $wavPath, string $mp3Path): void
    {
        if (is_file($wavPath)) {
            $command = sprintf(
                '%s -i %s -ab 320k -joint_stereo 0 -ac 2 %s 2>&1',
                escapeshellcmd($this->converterPath),
                escapeshellarg($wavPath),
                escapeshellarg($mp3Path),
            );
            exec($command);
            if (!is_file($wavPath)) {
                throw new Exception("Could not convert wave file {$wavPath} to mp3");
            }
        }
    }
}