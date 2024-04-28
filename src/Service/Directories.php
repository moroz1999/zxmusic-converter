<?php

namespace ZxMusic\Service;

use RuntimeException;

class Directories
{
    public function prepareDirectory(string $resultPath): void
    {
        if (!is_dir($resultPath) && !mkdir($resultPath, 0777, true) && !is_dir($resultPath)) {
            throw new RuntimeException(sprintf('Directory "%s" was not created', $resultPath));
        }
    }
}