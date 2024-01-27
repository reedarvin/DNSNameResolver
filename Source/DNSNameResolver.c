//
// gcc source.c -o source.exe -lws2_32
//

#define _WIN32_WINNT 0x0500
#define WINVER       0x0500

#define STRICT
#define WIN32_LEAN_AND_MEAN

#define _WINSOCKAPI_

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <winsock2.h>

#define MAX_THREADS 64

VOID    ThreadedSub( VOID *pParameter );
VOID ResolveDNSName( CHAR *szTarget, BOOL *bMultipleHosts );

typedef struct _THREAD_ARGS
{
	CHAR        Target[ 128 ];
	BOOL MultipleHosts;
} THREAD_ARGS, *PTHREAD_ARGS;

HANDLE hSemaphore;

INT nThreads = 0;

INT main( INT argc, CHAR *argv[] )
{
	CHAR szTargetInput[ 128 ];
	FILE   *pInputFile;
	CHAR    szReadLine[ 128 ];
	CHAR      szTarget[ 128 ];

	PTHREAD_ARGS pThreadArgs;

	hSemaphore = CreateSemaphore( NULL, 1, 1, NULL );

	if ( argc == 2 )
	{
		strcpy( szTargetInput, argv[1] );

		printf( "Running DNSNameResolver v1.1 with the following arguments:\n" );
		printf( "[+] Host Input:   \"%s\"\n", szTargetInput );
		printf( "[+] # of Threads: \"64\"\n" );
		printf( "[+] Output File:  \"DNSNameResolver.txt\"\n" );
		printf( "\n" );

		pInputFile = fopen( szTargetInput, "r" );

		if ( pInputFile != NULL )
		{
			while ( fscanf( pInputFile, "%s", szReadLine ) != EOF )
			{
				strcpy( szTarget, szReadLine );

				while ( nThreads >= MAX_THREADS )
				{
					Sleep( 200 );
				}

				pThreadArgs = (PTHREAD_ARGS)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( THREAD_ARGS ) );

				if ( pThreadArgs != NULL )
				{
					strcpy( pThreadArgs->Target, szTarget );

					pThreadArgs->MultipleHosts = TRUE;

					WaitForSingleObject( hSemaphore, INFINITE );

					nThreads++;

					ReleaseSemaphore( hSemaphore, 1, NULL );

					_beginthread( ThreadedSub, 0, (VOID *)pThreadArgs );
				}
			}

			fclose( pInputFile );

			Sleep( 1000 );

			printf( "Waiting for threads to terminate...\n" );
		}
		else
		{
			strcpy( szTarget, szTargetInput );

			pThreadArgs = (PTHREAD_ARGS)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( THREAD_ARGS ) );

			if ( pThreadArgs != NULL )
			{
				strcpy( pThreadArgs->Target, szTarget );

				pThreadArgs->MultipleHosts = FALSE;

				WaitForSingleObject( hSemaphore, INFINITE );

				nThreads++;

				ReleaseSemaphore( hSemaphore, 1, NULL );

				_beginthread( ThreadedSub, 0, (VOID *)pThreadArgs );
			}
		}

		while ( nThreads > 0 )
		{
			Sleep( 200 );
		}
	}
	else
	{
		printf( "DNSNameResolver v1.1 | https://github.com/reedarvin\n" );
		printf( "\n" );
		printf( "Usage: DNSNameResolver <hostname | hostname input file>\n" );
		printf( "\n" );
		printf( "<hostname | ip input file>  -- required argument\n" );
		printf( "\n" );
		printf( "Examples:\n" );
		printf( "DNSNameResolver MyWindowsMachine\n" );
		printf( "DNSNameResolver IPInputFile.txt\n" );
		printf( "\n" );
		printf( "(Written by Reed Arvin | reedlarvin@gmail.com)\n" );
	}

	CloseHandle( hSemaphore );

	return 0;
}

VOID ThreadedSub( VOID *pParameter )
{
	CHAR       szTarget[ 128 ];
	BOOL bMultipleHosts;

	PTHREAD_ARGS pThreadArgs;

	pThreadArgs = (PTHREAD_ARGS)pParameter;

	strcpy( szTarget, pThreadArgs->Target );

	bMultipleHosts = pThreadArgs->MultipleHosts;

	HeapFree( GetProcessHeap(), 0, pThreadArgs );

	if ( bMultipleHosts )
	{
		printf( "Spawning thread for host %s...\n", szTarget );
	}

	ResolveDNSName( szTarget, &bMultipleHosts );

	WaitForSingleObject( hSemaphore, INFINITE );

	nThreads--;

	ReleaseSemaphore( hSemaphore, 1, NULL );

	_endthread();
}

VOID ResolveDNSName( CHAR szTarget[], BOOL *bMultipleHosts )
{
	INT          nResult;
	WSADATA      wsaData;
	CHAR     szIPAddress[ 16 ];
	FILE    *pOutputFile;

	struct hostent *remoteHost;

	nResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );

	if ( nResult == NO_ERROR )
	{
		remoteHost = gethostbyname( szTarget );

		if ( remoteHost != NULL )
		{
			strcpy( szIPAddress, inet_ntoa( *(struct in_addr *)remoteHost->h_addr_list[0] ) );

			if ( !*bMultipleHosts )
			{
				printf( "\n" );
				printf( "+++++++++++++++++++++++++++++++++++++++++++++++++++\n" );
				printf( "+++++            DNS NAME RESOLVER            +++++\n" );
				printf( "+++++++++++++++++++++++++++++++++++++++++++++++++++\n" );
				printf( "\n" );

				printf( "IP Address: %s\n", szIPAddress );
			}

			WaitForSingleObject( hSemaphore, INFINITE );

			pOutputFile = fopen( "DNSNameResolver.txt", "r" );

			if ( pOutputFile != NULL )
			{
				fclose( pOutputFile );
			}
			else
			{
				pOutputFile = fopen( "DNSNameResolver.txt", "w" );

				if ( pOutputFile != NULL )
				{
					fprintf( pOutputFile, "NOTE: This file is tab separated. Open with Excel to view and sort information.\n" );
					fprintf( pOutputFile, "\n" );
					fprintf( pOutputFile, "Hostname\tIP Address\n" );

					fclose( pOutputFile );
				}
			}

			pOutputFile = fopen( "DNSNameResolver.txt", "a+" );

			if ( pOutputFile != NULL )
			{
				fprintf( pOutputFile, "%s\t%s\n", szTarget, szIPAddress );

				fclose( pOutputFile );
			}

			ReleaseSemaphore( hSemaphore, 1, NULL );
		}
		else
		{
			if ( !*bMultipleHosts )
			{
				fprintf( stderr, "ERROR! Cannot resolve host %s to an IP address.\n", szTarget );
			}
		}
	}

	WSACleanup();
}

// Written by Reed Arvin | reedlarvin@gmail.com
