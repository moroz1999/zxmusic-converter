<?php
declare(strict_types=1);

namespace ZxMusic\Service\Arkos;

use ZipArchive;
use ZxMusic\Exception\ConversionException;

readonly class AksInformationParser
{
    /**
     * @throws ConversionException
     */
    public function getAksInformation(string $path): ParsedInformation
    {
        $xmlContent = $this->extractAksFile($path);
        return $this->parseAksXml($xmlContent);
    }

    /**
     * @throws ConversionException
     */
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

        throw new ConversionException('AKS file format not recognized (expected ZIP or GZIP): ' . basename($path));
    }

    /**
     * @throws ConversionException
     */
    private function extractFromZip(string $zipFilePath): string
    {
        $zip = new ZipArchive();
        if ($zip->open($zipFilePath) !== true) {
            throw new ConversionException('Failed to open ZIP archive: ' . basename($zipFilePath));
        }

        $extractedContent = $zip->getFromName($zip->getNameIndex(0));
        $zip->close();

        if ($extractedContent === false) {
            throw new ConversionException('Failed to extract content from ZIP archive: ' . basename($zipFilePath));
        }

        return $extractedContent;
    }

    /**
     * @throws ConversionException
     */
    private function extractFromGzip(string $gzipFilePath): string
    {
        $bufferSize = 4096;
        $file = gzopen($gzipFilePath, 'rb');
        if ($file === false) {
            throw new ConversionException('Failed to open GZIP file: ' . basename($gzipFilePath));
        }

        $extractedContent = '';
        while (!gzeof($file)) {
            $extractedContent .= gzread($file, $bufferSize);
        }
        gzclose($file);

        return $extractedContent;
    }

    /**
     * @throws ConversionException
     */
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
                    null,
                    null
                );
            }
            if ($xml->Version !== null) {
                return new ParsedInformation(
                    (string)($xml->Name ?? ''),
                    (string)($xml->Author ?? ''),
                    Version::VERSION1,
                    (int)($xml->MasterFrequency ?? 1770000),
                    (string)($xml->Version)
                );
            }
        }

        $errors = array_map(static fn($error) => $error->message, libxml_get_errors());
        libxml_clear_errors();
        throw new ConversionException('AKS XML parsing failed: ' . implode(', ', $errors));
    }
}
