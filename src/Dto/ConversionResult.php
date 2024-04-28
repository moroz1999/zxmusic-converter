<?php

declare(strict_types=1);

namespace ZxMusic\Dto;

use JsonSerializable;

readonly class ConversionResult implements JsonSerializable
{
    public function __construct(
        public string $mp3Name,
        public string $convertedFile,
        public string $title,
        public string $author,
        public string $time,
        public string $channels,
        public string $type,
        public string $container,
        public string $program
    )
    {
    }

    public function jsonSerialize(): array
    {
        return [
            'mp3Name' => $this->mp3Name,
            'title' => $this->title,
            'author' => $this->author,
            'time' => $this->time,
            'channels' => $this->channels,
            'type' => $this->type,
            'container' => $this->container,
            'program' => $this->program
        ];
    }
}
