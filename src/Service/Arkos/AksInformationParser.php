<?php
declare(strict_types=1);


namespace ZxMusic\Service\Arkos;

use RuntimeException;
use ZipArchive;

class AksInformationParser
{
    public function getAksInformation(string $path): ParsedInformation
    {
        $xmlContent = $this->extractAksFile($path);
        return $this->parseAksXml($xmlContent);
    }

    private function extractAksFile(string $path): string
    {
        $zip = new ZipArchive();
        if ($zip->open($path) === TRUE) {
            $extractedContent = $zip->getFromName($zip->getNameIndex(0));
            $zip->close();
            if ($extractedContent === false) {
                throw new RuntimeException('File extraction has failed');
            }
            return $extractedContent;
        }

        throw new RuntimeException('Zip file is not recognized.');
    }

    private function parseAksXml(string $xmlContent): ParsedInformation
    {
        libxml_use_internal_errors(true);
        $xml = simplexml_load_string($xmlContent);

        if ($xml !== false) {
            $namespaces = $xml->getNamespaces(true);
            $aks = $xml->children($namespaces['aks']);

            $title = (string)($aks->title ?? '');
            $author = (string)($aks->author ?? '');
            $formatVersion = (string)($aks->formatVersion ?? '');

            return new ParsedInformation(
                $title,
                $author,
                $this->convertToVersion($formatVersion)
            );
        }

        $errors = array_map(static fn($error) => $error->message, libxml_get_errors());
        libxml_clear_errors();
        throw new RuntimeException('XML parsing error: ' . implode(', ', $errors));
    }

    private function convertToVersion(string $version): Version
    {
        return match ($version) {
            '1.0' => Version::VERSION1,
            '2.0' => Version::VERSION2,
            default => throw new RuntimeException("Unsupported Arkos version: {$version}"),
        };
    }
}

