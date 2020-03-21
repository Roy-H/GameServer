//
//  HttpReq.cpp
//  HttpClient
//
//  Created by LiYong on 2018/1/23.
//

#include "HttpReq.h"
HttpRes::HttpRes()
{
    m_iSocketFd = INVALID_SOCKET;
}
HttpRes::~HttpRes()
{

}
HttpRes *HttpRes::getInstance()
{
    HttpRes *http = new HttpRes();
    if (http)
    {
        return http;
    }
    return nullptr;
}
int HttpRes::httpGet(string strUrl, string &strResponse)
{
    return httpResquestExec("GET", strUrl, "", strResponse);
}
int HttpRes::httpPost(string strUrl,string strData, string &strResponse)
{
    return httpResquestExec("POST",strUrl, strData, strResponse);
}
int HttpRes::httpResquestExec(string strMethod,string strUrl,string strData, string &strResponse)
{
    //判断URL是否有效
    if (strUrl == "")
    {
        debugOut("URL为空\n");
        return 0;
    }
    //限制URL的长度
    if (URLSIZE < strUrl.size())
    {
        debugOut("URL的长度不能超过：%d\n",URLSIZE);
        return 0;
    }

    //创建HTTP协议表头
    string strHttpHead = httpHeadCreate(strMethod, strUrl, strData);

    if (m_iSocketFd != INVALID_SOCKET)
    {
        string strResult = httpDataTransmit(strHttpHead, m_iSocketFd);
        if (strResult != "")
        {
            strResponse = strResult;
            return 1;
        }
    }
    //创建Socket
    m_iSocketFd = INVALID_SOCKET;
    m_iSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_iSocketFd < 0)
    {
        debugOut("socket error! Error code: %d,Error message: %s\n",errno,strerror(errno));
        return 0;
    }

    //绑定地址端口
    int iPort = getPortFromUrl(strUrl);
    if (iPort < 0)
    {
        debugOut("获取URL端口失败\n");
        return 0;
    }
    string strIP = getIpFromUrl(strUrl);
    if (strIP == "")
    {
        debugOut("从URL获取IP地址失败\n");
        return 0;
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(iPort);
    if (inet_pton(AF_INET,strIP.data(), &servaddr.sin_addr) <= 0)
    {
        debugOut("inet_pton error ! Error code: %d,Error message:%s\n",errno,strerror(errno));
        close(m_iSocketFd);
        m_iSocketFd = INVALID_SOCKET;
        return 0;
    }

    int flags = fcntl(m_iSocketFd, F_SETFL, 0);
    if (fcntl(m_iSocketFd, F_SETFL,flags|O_NONBLOCK) == -1)
    {
        close(m_iSocketFd);
        m_iSocketFd = INVALID_SOCKET;
        debugOut("fcntl error! Error code: %d,Error message: %s",errno,strerror(errno));
        return 0;
    }

    //非阻塞方式连接
    int iRet = connect(m_iSocketFd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (iRet == 0)
    {
        string strResult = httpDataTransmit(strHttpHead, m_iSocketFd);
        if (NULL == strResult.c_str())
        {
            close(m_iSocketFd);
            m_iSocketFd = INVALID_SOCKET;
            return 0;
        }
        else
        {
            strResponse = strResult;
            return 1;
        }
    }
    else if(iRet < 0)
    {
        if (errno != EINPROGRESS)
        {
            return 0;
        }
    }

    iRet = socketFdCheck(m_iSocketFd);
    if (iRet > 0)
    {
        string strResult = httpDataTransmit(strHttpHead, m_iSocketFd);
        if (strResult == "")
        {
            close(m_iSocketFd);
            m_iSocketFd = INVALID_SOCKET;
            return 0;
        }
        else
        {
            strResponse = strResult;

            return 1;
        }
    }
    else
    {
        close(m_iSocketFd);
        m_iSocketFd = INVALID_SOCKET;
        return 0;
    }
    return 1;
}
string HttpRes::httpHeadCreate(string strMethod,string strUrl,string strData)
{
    string strHost = getHostAddFromUrl(strUrl);
    string strParam = getParamFromUrl(strUrl);

    string strHttpHead;
    strHttpHead.append(strMethod);
    strHttpHead.append(" /");
    strHttpHead.append(strParam);
    strHttpHead.append(" HTTP/1.1\r\n");
    strHttpHead.append("Accept: */*\r\n");
    strHttpHead.append("Accept-Language: cn\r\n");
    strHttpHead.append("User-Agent: Mozilla/4.0\r\n");
    strHttpHead.append("Host: ");
    strHttpHead.append(strHost);
    strHttpHead.append("\r\n");
    strHttpHead.append("Cache-Control: no-cache\r\n");
    strHttpHead.append("Connection: Keep-Alive\r\n");
    if (strMethod == "POST")
    {
        char len[8] = {0};
        unsigned long iLen = strData.size();
        sprintf(len, "%lu",iLen);

        strHttpHead.append("Content-Type: application/x-www-form-urlencoded\r\n");
        strHttpHead.append("Content-Length: ");
        strHttpHead.append(len);
        strHttpHead.append("\r\n\r\n");
        strHttpHead.append(strData);
    }
    strHttpHead.append("\r\n\r\n");

    return strHttpHead;
}
//发送HTTP请求并且接受响应
string HttpRes::httpDataTransmit(string strHttpHead,int isSocFd)
{
    char *buf = (char *)malloc(BUFSIZ);
    memset(buf, 0, BUFSIZ);
    char *head = (char *)strHttpHead.data();
    long ret = send(isSocFd, (void *)head, strlen(head)+1, 0);
    if (ret < 0)
    {
        debugOut("send error ! Error code: %d,Error message: %s\n",errno,strerror(errno));
        close(isSocFd);
        return nullptr;
    }
    while (1)
    {
        ret = recv(isSocFd, (void *)buf, BUFSIZ, 0);
        if (ret == 0)
        {
            close(isSocFd);
            free(buf);
            return nullptr;
        }
        else if (ret > 0)
        {
            string strRecv = buf;
            free(buf);
            return strRecv;
        }
        else if (ret < 0)
        {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            {
                continue;
            }
            else
            {
                close(isSocFd);
                free(buf);
                return nullptr;
            }
        }
    }
}
//从URL中获取Host地址
string HttpRes::getHostAddFromUrl(string strUrl)
{

    char url[URLSIZE] = {0};
    strcpy(url, strUrl.c_str());
    char *strAddr = strstr(url, "http://");
    if (strAddr == NULL)
    {
        strAddr = strstr(url, "https://");
        if (strAddr != NULL)
        {
            strAddr += 8;
        }
    }
    else
    {
        strAddr += 7;
    }
    if (strAddr == NULL)
    {
        strAddr = url;
    }

    char * strHostAddr = (char *)malloc(strlen(strAddr) +1);

    memset(strHostAddr, 0, strlen(strAddr) + 1);
    for (int i = 0; i < strlen(strAddr) + 1; i++)
    {
        if (strAddr[i] == '/')
        {
            break;
        }
        else
        {
            strHostAddr[i] = strAddr[i];
        }
    }
    string host = strHostAddr;
    free(strHostAddr);
    return host;
}
//从HTTP请求URL中获取HTTP请求参数
string HttpRes::getParamFromUrl(string strUrl)
{
    char url[URLSIZE] = {0};
    strcpy(url, strUrl.c_str());

    char *strAddr = strstr(url, "http://");
    if (strAddr == NULL)
    {
        strAddr = strstr(url, "https://");
        if (strAddr != NULL)
        {
            strAddr += 8;
        }
    }
    else
    {
        strAddr += 7;
    }
    if (strAddr == NULL)
    {
        strAddr = url;
    }

    char *strParam = (char *)malloc(strlen(strAddr)+1);
    memset(strParam, 0, strlen(strAddr)+1);
    int iPos = -1;
    for (int i = 0; i < strlen(strAddr)+1; i++)
    {
        if (strAddr[i] == '/')
        {
            iPos = i;
            break;
        }
    }
    if (iPos == -1)
    {
        strcpy(strParam, "");
    }
    else
    {
        strcpy(strParam, strAddr+iPos+1);
    }
    string param = strParam;
    free(strParam);
    return param;
}
//从HTTP请求URL中获取端口号
int HttpRes::getPortFromUrl(string strUrl)
{
    int nPort = -1;
    char *strHostAddr = (char *)getHostAddFromUrl(strUrl).data();

    if (strHostAddr == NULL)
    {
        return -1;
    }

    char strAddr[URLSIZE] = {0};
    strcpy(strAddr, strHostAddr);
    char *strPort = strchr(strAddr, ':');
    if (strPort == NULL)
    {
        nPort = 80;
    }
    else
    {
        nPort = atoi(++strPort);
    }
    return nPort;
}
//从Http请求URL中获取IP地址
string HttpRes::getIpFromUrl(string strUrl)
{

    string url = getHostAddFromUrl(strUrl);
    char *strHostAddr = (char *)url.data();
    char *strAddr = (char *)malloc(strlen(strHostAddr) + 1);
    memset(strAddr, 0, strlen(strAddr)+1);
    int nCount = 0;
    int nFlag = 0;
    for (int i = 0; i < strlen(strAddr) + 1; i++)
    {
        if (strHostAddr[i] == ':')
        {
            break;
        }
        strAddr[i] = strHostAddr[i];
        if (strHostAddr[i] == '.')
        {
            nCount++;
            continue;
        }
        if (nFlag == 1)
        {
            continue;
        }
        if ((strHostAddr[i] >= 0)||(strHostAddr[i] <= '9'))
        {
            nFlag = 0;
        }
        else
        {
            nFlag = 1;
        }
    }
    if (strlen(strAddr) <= 1)
    {
        return NULL;
    }

    //判断是否为点分十进制IP,否则通过域名获取IP
    if ((nCount == 3) && (nFlag == 0))
    {
        return strAddr;
    }
    else
    {
        struct hostent *he = gethostbyname(strAddr);
        free(strAddr);
        if (he == NULL)
        {
            return NULL;
        }
        else
        {
            struct in_addr** addr_list = (struct in_addr **)he->h_addr_list;
            for (int i = 0; addr_list[i] != NULL; i++)
            {
                return inet_ntoa(*addr_list[i]);
            }
            return NULL;
        }
    }
}
//检查socketFd是否为可写不可读状态
int HttpRes::socketFdCheck(const int iSockFd)
{
    struct timeval timeout ;
    fd_set rset,wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(iSockFd, &rset);
    FD_SET(iSockFd, &wset);
    timeout.tv_sec = 3;
    timeout.tv_usec = 500;
    int iRet = select(iSockFd+1, &rset, &wset, NULL, &timeout);
    if(iRet > 0)
    {
        //判断SocketFd是否为可写不可读状态
        int iW = FD_ISSET(iSockFd,&wset);
        int iR = FD_ISSET(iSockFd,&rset);
        if(iW && !iR)
        {
            char error[4] = "";
            socklen_t len = sizeof(error);
            int ret = getsockopt(iSockFd,SOL_SOCKET,SO_ERROR,error,&len);
            if(ret == 0)
            {
                if(!strcmp(error, ""))
                {
                    return iRet;//表示已经准备好的描述符数
                }
                else
                {
                    debugOut("%s %s %d\tgetsockopt error code:%d,error message:%s", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
                }
            }
            else
            {
                debugOut("%s %s %d\tgetsockopt failed. error code:%d,error message:%s", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            }
        }
        else
        {
            debugOut("%s %s %d\tsockFd是否在可写字符集中：%d，是否在可读字符集中：%d\t(0表示不在)\n", __FILE__, __FUNCTION__, __LINE__, iW, iR);
        }
    }
    else if(iRet == 0)
    {
        return 0;//表示超时
    }
    else
    {
        return -1;//select出错，所有描述符集清0
    }
    return -2;//其他错误
}
void HttpRes::debugOut(string fmt, ...)
{
#ifdef __DEBUG__
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt.c_str(), ap);
    va_end(ap);
#endif
}
int HttpRes::m_iSocketFd = INVALID_SOCKET;
