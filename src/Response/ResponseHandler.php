<?php

declare(strict_types=1);

namespace ZxMusic\Response;

use JsonException;
use ZxMusic\Dto\ApiResponse;
use function strlen;

class ResponseHandler
{
    /**
     * @throws JsonException
     */
    private function sendResponse(ApiResponse $response): void
    {
        $jsonData = json_encode($response, JSON_THROW_ON_ERROR);
        $gzipContent = gzencode($jsonData, 3);

        header('Content-Type: application/json');
        header('Content-Encoding: gzip');
        header('Content-Length: ' . strlen($gzipContent));
        echo $gzipContent;
        exit;
    }

    /**
     * @throws JsonException
     */
    public function sendSuccess(array $data): void
    {
        $response = new ApiResponse(true, $data);
        $this->sendResponse($response);
    }

    /**
     * @throws JsonException
     */
    public function sendError(string $message, int $statusCode = 400): void
    {
        http_response_code($statusCode);
        $response = new ApiResponse(false, [], $message);
        $this->sendResponse($response);
    }
}
