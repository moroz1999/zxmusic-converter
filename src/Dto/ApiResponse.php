<?php

declare(strict_types=1);

namespace ZxMusic\Dto;

use JsonSerializable;

readonly class ApiResponse implements JsonSerializable
{
    public function __construct(
        private bool    $success,
        private array   $data,
        private ?string $error = null
    )
    {
    }

    public function jsonSerialize(): array
    {
        return [
            'success' => $this->success,
            'data' => $this->data,
            'error' => $this->error
        ];
    }
}
