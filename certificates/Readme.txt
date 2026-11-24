Following certificates were created with following commands:
MakeCert -r -pe -ss PrivateCertStore -n "CN=DekuchampTest" -sv DekuchampTestCert.pvk DekuchampTestCert.cer
pvk2pfx -pvk DekuchampTestCert.pvk -spc DekuchampTestCert.cer -pfx DekuchampTestCert.pfx

Password: Dekuchamp


WARNING: Prior to installing the test-signed driver you need to run:
CertMgr /add DekuchampTestCert.cer /s /r localMachine root
CertMgr /add DekuchampTestCert.cer /s /r localMachine trustedpublisher
Bcdedit.exe -set TESTSIGNING ON
