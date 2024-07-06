<?php
declare(strict_types=1);

namespace ZxMusic\Service\Converter;

use InvalidArgumentException;
use ZxMusic\Service\Arkos\Version;
use ZxMusic\Service\Arkos\VersionResolver;

readonly final class ConverterTypeResolver
{
    public function __construct(private VersionResolver $versionResolver)
    {

    }

    /**
     * Resolves the converter type based on the file extension.
     *
     * @param string $filePath The file path.
     * @return ConverterType The converter type.
     * @throws InvalidArgumentException If the file extension is not supported.
     */
    public function resolve(string $filePath): ConverterType
    {
        $extension = strtolower(pathinfo($filePath, PATHINFO_EXTENSION));

        $type = match ($extension) {
            'fur' => ConverterType::FURNACE,
            'aks' => ConverterType::ARKOS2,
            'chp' => ConverterType::CHIPNSFX,
            default => ConverterType::ZXTUNE
        };

        if ($type === ConverterType::ARKOS2) {
            $version = $this->versionResolver->resolveArkosVersion($filePath);
            return $version === Version::VERSION1 ? ConverterType::ARKOS1 : ConverterType::ARKOS2;
        }
        return $type;
    }
}