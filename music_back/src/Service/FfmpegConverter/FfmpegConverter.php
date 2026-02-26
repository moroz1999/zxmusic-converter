<?php

declare(strict_types=1);

namespace ZxMusic\Service\FfmpegConverter;

use ZxMusic\Exception\ConversionException;

readonly class FfmpegConverter
{
    public function __construct(
        private string $converterPath,
    )
    {
    }

    /**
     * @throws ConversionException
     */
    public function convertToMp3(string $wavPath, string $mp3Path): void
    {
        if (!is_file($wavPath)) {
            throw new ConversionException('WAV file not found for ffmpeg conversion: ' . $wavPath);
        }

        $output = [];
        $command = sprintf(
            '%s -i %s -ab 320k -joint_stereo 0 -ac 2 %s 2>&1',
            escapeshellcmd($this->converterPath),
            escapeshellarg($wavPath),
            escapeshellarg($mp3Path),
        );
        exec($command, $output);

        if (!is_file($mp3Path)) {
            throw new ConversionException(
                'ffmpeg failed to produce MP3 from: ' . basename($wavPath) . PHP_EOL . implode(PHP_EOL, $output)
            );
        }
    }
}
