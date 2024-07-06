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
        $fileInfo = new \finfo(FILEINFO_MIME_TYPE);
        $mimeType = $fileInfo->file($path);

        if ($mimeType === 'application/zip') {
            return $this->extractFromZip($path);
        }

        if ($mimeType === 'application/gzip') {
            return $this->extractFromGzip($path);
        }

        throw new RuntimeException('Zip file is not recognized.');
    }


    private function extractFromZip(string $zipFilePath): string
    {
        $zip = new ZipArchive();
        if ($zip->open($zipFilePath) === TRUE) {
            $extractedContent = $zip->getFromName($zip->getNameIndex(0));
            $zip->close();
            if ($extractedContent === false) {
                throw new RuntimeException("Ошибка: Не удалось извлечь файл из ZIP архива.");
            }
            return $extractedContent;
        }

        throw new RuntimeException("Ошибка: Это не ZIP файл.");
    }

    private function extractFromGzip(string $gzipFilePath): string
    {
        $bufferSize = 4096; // Размер буфера чтения
        $file = gzopen($gzipFilePath, 'rb');
        if ($file === false) {
            throw new RuntimeException("Ошибка: Не удалось открыть GZIP файл.");
        }

        $extractedContent = '';
        while (!gzeof($file)) {
            $extractedContent .= gzread($file, $bufferSize);
        }
        gzclose($file);

        return $extractedContent;
    }

    private function parseAksXml(string $xmlContent): ParsedInformation
    {
        libxml_use_internal_errors(true);
        $xml = simplexml_load_string($xmlContent);

        if ($xml !== false) {
            $namespaces = $xml->getNamespaces(true);
            if (isset($namespaces['aks'])) {
                $aks = $xml->children($namespaces['aks']);
                return new ParsedInformation(
                    (string)($aks->title ?? ''),
                    (string)($aks->author ?? ''),
                    Version::VERSION2,
                    null
                );
            }
            if ($xml->Version !== null) {
                return new ParsedInformation(
                    (string)($xml->Name ?? ''),
                    (string)($xml->Author ?? ''),
                    Version::VERSION1,
                    (int)($xml->MasterFrequency ?? 1770000)
                );
            }
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

