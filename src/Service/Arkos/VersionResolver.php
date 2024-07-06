<?php
declare(strict_types=1);

namespace ZxMusic\Service\Arkos;


class VersionResolver
{
    public function __construct(private AksInformationParser $aksInformationParser)
    {

    }

    public function resolveArkosVersion(string $path): ?Version
    {
        return $this->aksInformationParser->getAksInformation($path)->formatVersion;
    }
}