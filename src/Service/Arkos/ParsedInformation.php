<?php
declare(strict_types=1);

namespace ZxMusic\Service\Arkos;

readonly class ParsedInformation
{
    public function __construct(
        public string  $title,
        public string  $author,
        public Version $formatVersion,
        public ?int    $frequency,
        public ?string $trackerVersion
    )
    {
    }
}

